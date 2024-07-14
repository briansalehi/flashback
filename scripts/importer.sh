#!/usr/bin/env bash

error() { tput setaf 1; tput bold; echo "$@" >&2; tput sgr0; exit 1; }
alert()
{
    tput bold
    if [ $# -eq 1 ]
    then
        tput setaf 5
        echo -e "$1"
    else
        tput setaf 4
        echo -ne "$1: "
        tput setaf 8
        echo -e "$2"
    fi
    tput sgr0
}
checkpoint_steps=0
checkpoint()
{
    checkpoint_steps="$((checkpoint_steps + 1))"

    [ "${DEBUG:-0}" -ge 2 ] || return
    [ "${SKIP_DEBUG:-0}" -le "$checkpoint_steps" ] || return

    if [ $# -eq 1 ]
    then
        echo -e "Step $checkpoint_steps\n$1" | less
    else
        local heading="$1"
        shift
        (
        echo -e "Step $checkpoint_steps\n>>> $heading <<<\n"
        for arg in "$@"; do echo "$arg"; done
        ) | less
    fi
    read -rp "Continue? " response </dev/tty
    [ "${response}" != "y" ] && exit 1
}
notice()
{
    [ "${DEBUG:-0}" -ge 1 ] || return
    alert "$1" "$2"
}
log()
{
    [ "${DEBUG:-0}" -ge 4 ] || return

    tput bold
    if [ $# -eq 1 ]
    then
        tput setaf 5
        echo -e "$1"
    else
        tput setaf 4
        echo -ne "$1: "
        tput setaf 8
        read -rp "$2" </dev/tty
    fi
    tput sgr0
}
report_progress()
{
    local counter="${1:-0}"
    local total="$3"
    local length

    if [ "$counter" -eq "${total:-0}" ]
    then
        length="$(echo -n "$((counter - 1))${total:+/}${total}" | wc -c)"
        printf "%.s\b" $(seq 1 "$length")
        [ "$total" -gt 1 ] && echo "$counter${total:+/}${total}"
        [ "$total" -eq 1 ] && echo "${2:-Untitled}: $counter${total:+/}${total}"
    elif [ "$counter" -eq 1 ]
    then
        echo -n "${2:-Untitled}: $counter${total:+/}${total}"
    else
        length="$(echo -n "$((counter - 1))${total:+/}${total}" | wc -c)"
        printf "%.s\b" $(seq 1 "$length")
        echo -n "$counter${total:+/}${total}"
    fi
}

declare -A subjects
declare -A subject_records
declare -a subject_entries

if [ ! -s "$PWD/design/database.sql" ]
then
    error "Database initial script cannot be found in $PWD/design/database.sql"
fi

if [ ! -d /tmp/references ]
then
    git clone https://github.com/briansalehi/references.git /tmp/references || error "Cannot clone repository"
fi

# recreate database
report_progress 1 "Creating Database Structures" 1
psql -q -U postgres -c "\i $PWD/design/database.sql" || error "Cannot recreate database"

subject_index=1
total_subjects=$(find /tmp/references/subjects -mindepth 1 -maxdepth 1 | wc -l)
for subject in /tmp/references/subjects/*
do
    report_progress $subject_index "Collecting Subjects" "$total_subjects"
    subject_path="$(basename "$subject")"
    subject_name="$(sed -n '1{s/#\s*//p}' "/tmp/references/subjects/${subject_path}/${subject_path}.md")"
    subject_entries+=( "${subject_entries:+, }('${subject_name}')" )
    subjects["${subject_path}"]="${subject_name}"
    subject_index=$((subject_index + 1))
done

subjects_query="insert into flashback.subjects (name) values ${subject_entries[*]};"
checkpoint "Check Subjects Query" "$subjects_query"

# import subjects
report_progress 1 "Storing Subjects" 1
psql -q -U postgres -d flashback -c "${subjects_query}" || error "Cannot store subjects"

declare -a topic_entries
subject_id=1
topic_index=1
total_topics="$(find /tmp/references/subjects/ -mindepth 2 -maxdepth 2 -type f -name '*.md' -not -name 'README.md' -exec grep '^## ' {} \; | wc -l)"
for subject in "${!subjects[@]}"
do
    while read -r topic
    do
        [ -z "$topic" ] && continue
        report_progress $topic_index "Collecting Topics" "$total_topics"
        subject_id=$(psql -U postgres -d flashback -At -c "select id from flashback.subjects where name = '${subjects[$subject]}';")
        subject_records[$subject_id]="${subjects[$subject]}"
        topic_entries+=( "${topic_entries[*]:+, }('${topic}', '${subject_id}')" )
        topic_index=$((topic_index + 1))
    done <<< "$(find "/tmp/references/subjects/${subject}" -mindepth 1 -maxdepth 1 -type f -name "${subject}.md" -exec grep -E '^##\s+\w+' {} \; | sed 's/^##\s//')"

    subject_id=0
    subject_index=$((subject_index + 1))
done
topic_index=$((topic_index - 1))

checkpoint "Check Subject Records" "${subject_records[@]}"

# import topics
report_progress 1 "Storing Topics" 1
topics_query="insert into flashback.topics (name, subject_id) values ${topic_entries[*]};"
checkpoint "Check Topics Query" "${topics_query}"
psql -q -U postgres -d flashback -c "${topics_query}" || error "Cannot store topics"

declare -A topic_records
topic_record_index=1
while read -r record
do
    report_progress $topic_record_index "Collecting Topic Indexes" $topic_index
    [ -z "${record#*|}" ] && continue
    [ -z "${record%|*}" ] && continue
    topic_records["${record#*|}"]="${record%|*}"
    topic_record_index="$((topic_record_index + 1))"
done <<< "$(psql -U postgres -d flashback -At -c 'select id, name from flashback.topics;' || error "Cannot collect topics")"
checkpoint "Check Topic Records with IDs" "$(for pair in "${!topic_records[@]}"; do echo "$pair = ${topic_records[$pair]}"; done)"

declare -A topic_subject_mapping
topic_subject_index=1
while read -r record
do
    report_progress $topic_subject_index "Collecting Topic-Subject Relation Maps" $topic_index
    [ -z "${record#*|}" ] && continue
    [ -z "${record%|*}" ] && continue
    topic_subject_mapping["${record%|*}"]="${record#*|}"
    topic_subject_index="$((topic_subject_index + 1))"
done <<< "$(psql -U postgres -d flashback -At -c 'select t.id, s.id from flashback.subjects s join flashback.topics t on t.subject_id = s.id;' || error "Cannot collect subject and topic identifiers")"
checkpoint "Check Subject-Topic Mappings" "$(for pair in "${!topic_subject_mapping[@]}"; do echo "$pair = ${topic_subject_mapping[$pair]}"; done)"

report_progress 1 "Collecting Resources" 1
declare -A resources
while read -r resource_dir
do
    while read -r resource_path
    do
        resource="$(sed -n '1s/#\s\+\(.*\)$/\1/p' "$resource_path" | sed 's/<sup>.*//')"
        resources["$resource_path"]="$resource"
    done <<< "$(find "$resource_dir" -type f -name '*.md')"
done <<< "$(find /tmp/references/subjects/ -mindepth 2 -maxdepth 2 -type d -name resources)"
checkpoint "Check Resource Entries with Resource Names" "$(for pair in "${!resources[@]}"; do echo "$pair = ${resources[$pair]}"; done)"

resources_query=""
resources_query_values=""
for resource in "${resources[@]}"
do
    [ -z "$resource" ] && continue
    resource="${resource//\'/\'\'}"
    resources_query_values="${resources_query_values}${resources_query_values:+ , }('${resource}')"
done
resources_query="insert into flashback.resources (name) values $resources_query_values;"

checkpoint "Check Resources Query" "$resources_query"

report_progress 1 "Storing Resources" 1
if ! psql -q -U postgres -d flashback -c "$resources_query"
then
    alert "values" "$resources_query_values"
    alert "query" "$resources_query"
    error "Failed to store resources"
fi

declare -A resources_mapping
resource_index=1
while read -r record
do
    report_progress $resource_index "Collecting Resource Identifiers" ${#resources[*]}
    [ -z "${record#*|}" ] && continue
    [ -z "${record%|*}" ] && continue
    resources_mapping["${record%|*}"]="${record#*|}"
    resource_index="$((resource_index + 1))"
done <<< "$(psql -U postgres -d flashback -At -c 'select name, id from flashback.resources;' || error "Cannot collect resources from database")"
checkpoint "Check Resource Mappings" "$(for pair in "${!resources_mapping[@]}"; do echo "$pair = ${resources_mapping[$pair]}"; done)"

resource_section_index=1
declare -a sections
for resource_path in "${!resources[@]}"
do
    sections=()
    sections_query_values=
    resource_name="${resources[$resource_path]}"
    resource_id="${resources_mapping[$resource_name]}"
    checkpoint "Check Resource Section Entries" "$(grep '## ' "$resource_path")"

    while read -r section
    do
        section="${section/\#\# }"
        state="$(sed 's/.*<sup>(\(.*\))<\/sup>/\1/' <<< "$section")"
        section="$(sed 's/^\(.*\)\([0-9]\+\)\/\([0-9]\+\)/\1\2/;{s/\s*<sup>.*//}' <<< "$section")"
        case "${state,,}" in
            ignored|ignore) state="ignored" ;;
            writing|editing) state="writing" ;;
            complete|completed) state="completed" ;;
            published|publish|released) state="completed" ;;
            *) state="open"
        esac
        sections+=( "$section" )
        sections_query_values="${sections_query_values}${sections_query_values:+ , }($resource_id, '${section}', '$state')"
    done <<< "$(grep '## ' "$resource_path")"

    sections_query="insert into flashback.resource_sections (resource_id, headline, state) values $sections_query_values;"
    [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check Resource Sections Query" "$sections_query"

    report_progress $resource_section_index "Storing Resource Sections" $((${#resources[*]} + 1))
    psql -q -U postgres -d flashback -c "$sections_query" || error "Failed to insert resource sections for [$resource_id] $resource_name"
    resource_section_index="$((resource_section_index + 1))"
done

practice_index=1
declare -a blocks=()
declare -a practice_resources=()
declare -a references=()
total_practices="$(find /tmp/references/subjects/ -mindepth 2 -maxdepth 2 -type f -name '*.md' -not -name 'README.md' -exec grep '<details>' {} \; | wc -l)"
for subject in "${!subjects[@]}"
do
    log "Subject"
    subject_file="/tmp/references/subjects/${subject}/${subject}.md"
    inside_block=0
    code_block=0
    current_topic=
    blocks=()
    practice_resources=()
    references=()
    related_resources=()
    related_resource=
    heading=

    current_subject="${subjects[$subject]}"
    for related_resource in "${resources[@]}"
    do
        if grep -q "$current_subject" <<< "$related_resource"
        then
            related_resources+=( "$related_resource" )
        fi
    done
    [ "${DEBUG:-0}" -ge 2 ] && checkpoint "Check Current Subject with Related Resources" "$current_subject" "<>" "${related_resources[@]}"

    while read -r line
    do
        line="${line#> }"
        line="${line#>}"
        log "Line" "$line"
        if grep -q '## ' <<< "$line"
        then
            log "Topic Header"
            current_topic="${line/\#\# /}"
            elif [ "$line" == "---" ]
            then
                log "Horizontal Line"
                resources_block=0
                references_block=0
                code_block=0
                continue
            elif grep -q '<details>' <<< "$line"
            then
                log "Block Begin"
                inside_block=1
            elif grep -q '<summary>' <<< "$line"
            then
                log "Heading"
                heading="${line:9:-10}"
                read -r line
            elif grep -q '</details>' <<< "$line"
            then
                log "End of Block"
                inside_block=0
                resources_block=0
                references_block=0
                code_block=0
                topic_id="${topic_records[$current_topic]}"
                heading="${heading//\'/\'\'}"

                report_progress $practice_index "Storing Practice" "$total_practices"
                practice_index=$((practice_index + 1))

                query="insert into flashback.practices (heading, topic_id) values ('${heading}', $topic_id) returning id;"
                [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check Practices Query" "${query}"

                practice_id="$(psql -U postgres -d flashback -Aqt -c "$query" || error "Practice failed to be inserted")"

                for record in "${blocks[@]}"
                do
                    block_type="${record%%:*}"
                    record="${record#*:}"
                    language="${record%%:*}"
                    block="${record#*:}"
                    block="${block//\'/\'\'}"

                    query="insert into flashback.practice_blocks (content, type, language, practice_id) values ('${block}', '${block_type}', '${language:-txt}', ${practice_id});"
                    [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check Practice Blocks Query" "${query}"

                    if ! psql -q -U postgres -d flashback -c "$query"
                    then
                        alert "\nPractice $practice_id"
                        alert "Query" "$query"
                        alert "Topic" "$topic_id $current_topic"
                        alert "Heading" "$heading"
                        alert "Block" "${blocks[*]}"
                        error "Practice block failed to be inserted"
                    fi
                done

                for record in "${practice_resources[@]}"
                do
                    record="${record//\'/\'\'}"
                    resource_name="${record% - *}"
                    section_headline="${record##* - }"
                    resource_id="${resources_mapping[$resource_name]}"
                    [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check Resource Name and Section Headline Relation" "[$resource_id] $resource_name <> $section_headline"

                    if [ -z "$section_headline" ]
                    then
                        alert "Section headline was not detected in '$record'"
                        read -rp "Enter section headline: " section_headline
                    fi

                    if [ -z "${resource_id}" ] && grep -Eq 'https://(www\.)?youtube.com' <<< "$record"
                    then
                        resource_id=1
                        resource_name="YouTube"
                        section_headline="$record"
                    elif [ -z "${resource_id}" ] && grep -Eq 'https://youtube.com' <<< "$record"
                    then
                        alert "Resource name '$record' contains a link but does not match: Practice [$practice_id]"
                    #elif [ -z "${resource_id}" ]
                    #then
                    #    alert "Resource name '$resource_name' did not have exact match but there are similars: '$resource_name':"
                    #    select resource_name in "${related_resources[@]}"
                    #    do
                    #        resource_id="${resources_mapping[$resource_name]}"
                    #        [ -n "$resource_id" ] && break
                    #    done </dev/tty
                    #    [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check Selected Resource" "$resource_name $resource_id"
                    elif [ -z "${resource_id}" ]
                    then
                        alert "Resource name '$record' did not have exact match: Practice [$practice_id]"
                    fi

                    query="select id from flashback.resource_sections where resource_id = $resource_id and headline = '$section_headline';"
                    if [ -n "${resource_id}" ] && [ -n "$section_headline" ]
                    then
                        section_id="$(psql -U postgres -d flashback -Aqt -c "$query" || error "Failed to retrieve section id for $section_headline")"
                    fi

                    query="insert into flashback.practice_resources (practice_id, section_id) values (${practice_id}, ${section_id});"
                    [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check Resource Sections Query" "$query"

                    if [ -n "$section_id" ] && ! psql -q -U postgres -d flashback -c "$query"
                    then
                        alert "\nPractice $practice_id"
                        alert "Query" "$query"
                        alert "Topic" "$topic_id $current_topic"
                        alert "Heading" "$heading"
                        alert "Block" "${blocks[*]}"
                        alert "Resources" "${practice_resources[*]}"
                        error "practice resources failed to be inserted"
                    fi
                done

                for record in "${references[@]}"
                do
                    record="${record//\'/\'\'}"
                    query="insert into flashback.references (origin, practice_id) values ('${record}', ${practice_id});"
                    [ "${DEBUG:-0}" -ge 3 ] && checkpoint "Check References Query" "$query"

                    if ! psql -q -U postgres -d flashback -c "$query"
                    then
                        alert "\nPractice $practice_id"
                        alert "Query" "$query"
                        alert "Topic" "$topic_id $current_topic"
                        alert "Heading" "$heading"
                        alert "Block" "${blocks[*]}"
                        alert "Resources" "${practice_resources[*]}"
                        alert "References" "${references[*]}"
                        error "Practice references failed to be inserted"
                    fi
                done

                notice "\nPractice $practice_id"
                notice "Topic" "$topic_id $current_topic"
                notice "Heading" "$heading"
                notice "Block" "${blocks[*]}"
                notice "Resources" "${practice_resources[*]}"
                notice "References" "${references[*]}"

                blocks=()
                practice_resources=()
                references=()
            elif [ "$inside_block" -eq 1 ] && grep -Eq '^([*]+)?Description([*:]+)?' <<< "$line"
            then
                log "Description Ignored"
                continue
            elif [ "$inside_block" -eq 1 ] && grep -Eq '^([*]+)?Resources([*:]+)?' <<< "$line"
            then
                log "Resources Begin"
                code_block=0
                resources_block=1
                practice_resources=()
            elif [ "$inside_block" -eq 1 ] && grep -Eq '^([*]+)?References([*:]+)?' <<< "$line"
            then
                log "References Begin"
                code_block=0
                references_block=1
                references=()
            elif [ "$inside_block" -eq 1 ] && [ "${resources_block:-0}" -eq 1 ]
            then
                log "Resource Record"
                stripped="${line/- /}"
                log "Stripped Resource" "$stripped"
                if [ "${stripped/ /}" == "" ]
                then
                    log "Empty Resource Ignored"
                else
                    practice_resources+=( "$stripped" )
                fi
                stripped=
            elif [ "$inside_block" -eq 1 ] && [ "${references_block:-0}" -eq 1 ]
            then
                log "Reference Record"
                stripped="${line/- /}"
                log "Stripped Reference" "$stripped"
                if [ "${stripped/ /}" == "" ]
                then
                    log "Empty Reference Ignored"
                else
                    references+=( "$stripped" )
                fi
                stripped=
            elif [ "$inside_block" -eq 1 ] && grep -Eq '`+\w+$' <<< "$line"
            then
                log "Code Block Begin"
                code_block=1
                language="${line##*\`}"
                log "Detected Language" "$language"
            elif [ "$inside_block" -eq 1 ] && [ "$code_block" -eq 1 ] && grep -Eq '[`]{3,7}$' <<< "$line"
            then
                log "Code Block End"
                blocks+=( "code:$language:$buffer" )
                buffer=
                language=
                code_block=0
            elif [ "$inside_block" -eq 1 ] && [ -z "$line" ]
            then
                log "Text Block End"
                if [ -n "$buffer" ]
                then
                    blocks+=( "text::$buffer" )
                fi
                buffer=
            elif [ "$inside_block" -eq 1 ]
            then
                log "Buffer Line"
                buffer="$(echo -e "${buffer}${buffer:+\n}${line:-\n}")"
        fi
    done < "$subject_file"
done

