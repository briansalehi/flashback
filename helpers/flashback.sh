#!/usr/bin/env bash

trap 'printf "\r\e[1;31m%-*s\e[0m\n" $(tput cols) "User Interrupted"; tput cnorm; exit 1' SIGINT

user=2

last_line=$(($(tput lines) - 2))
margin=4

get_key()
{
    key=""
    action=""
    local -A keys
    local legend

    if [ $(($# % 2)) -ne 0 ]
    then
        prompt "Keys error: incorrect number of pairs '$*'"
        exit 1
    fi

    while [ $# -gt 0 ]
    do
        key="$1"
        action="$2"
        keys[$key]="$action"
        legend="${legend}${legend:+, }[$key] to $action"
        shift 2
    done

    prompt "Press ${legend}"

    while true
    do
        read -r -n 1 -s response </dev/tty

        if [ -n "$response" ] && [ -n "${keys[$response]}" ]
        then
            key="$response"
            action="${keys[$response]}"
            break
        fi
    done
    tput cnorm
}

custom_pager()
{
    local buffer
    buffer="$(mktemp)"

    cat > "$buffer"

    local line_count=0
    line_count="$(wc -l < "$buffer")"

    local terminal_height=0
    terminal_height=$(tput lines)

    if [ "$line_count" -gt "$terminal_height" ]
    then
        less -RFX < "$buffer"
    else
        cat "$buffer"
    fi

    rm -f "$buffer"
}

dense_column()
{
    local line=0
    local max_line_width=0
    local one_to_last_longest_line_width=0
    local max_width=0
    local total_lines=0
    local max_columns=0
    local column_number=0
    local line_number=0
    local remaining_line=0
    local line_length=0
    local line_bytes=0
    local last_column=0
    local buffer=""
    local max_lines=0

    buffer="$(mktemp)"

    while read -r line
    do
        echo "$line" >> "$buffer"
        total_lines=$((total_lines + 1))
        line_length="$(($(sed -E 's/\x1B\[[0-9;]*[mK]//g' <<< "$line" | wc -c) + margin))"
        [[ "${line_length}" -gt "$max_line_width" ]] && max_line_width="${line_length}" && line_bytes=${#line}
        [[ "${line_length}" -gt "$one_to_last_longest_line_width" ]] && [[ "${line_length}" -lt "$max_line_width" ]] && one_to_last_longest_line_width="${line_length}"
    done

    max_width=$(tput cols)
    max_width=$((max_width - margin))
    max_line_width=$((max_line_width - margin))
    line_bytes=$((line_bytes + 1))
    max_columns=$((max_width / max_line_width))
    [[ $((max_width % max_line_width)) -le $margin ]] && max_columns=$((max_columns + 1))
    [[ $((total_lines % max_columns)) -gt 0 ]] && remaining_line=1
    max_lines=$((total_lines / max_columns + remaining_line))

    for line_number in $(seq 1 $max_lines)
    do
        for column_number in $(seq "$line_number" $max_lines $total_lines)
        do
            last_column=$(seq "$line_number" $max_lines $total_lines | tail -n1)
            line="$(sed -n "$((column_number))p" "$buffer")"

            if [[ $column_number -eq $last_column ]]
            then
                printf "%-$((line_bytes - margin))s" "$line"
            else
                printf "%-${line_bytes}s" "$line"
            fi
        done
        echo
    done

    rm "$buffer"
}

language_is_supported()
{
    local extension="$1"

    if bat --list-languages | awk -F ':' '{print $2}' | tr ',' '\n' | grep -q "$extension"
    then
        return 0
    else
        return 1
    fi
}

capture_line()
{
    IFS=';' read -rsdR -p $'\E[6n' current_line _ </dev/tty
    current_line="${current_line#*[}"
}

show_blocks()
{
    local card="$1"
    local block
    local type
    local content
    local extension

    while IFS="|" read -r block type extension
    do
        content="$(psql -U flashback -d flashback -c "select content from blocks where card = $card and position = $block" -At)"

        printf "\e[2;37m%*s\e[0m\n" "$(tput cols)" "$type $extension $block"
        case "$type" in
            text)
                echo "$content" | bat --color always --paging never --squeeze-blank --language "md" --style "plain"
                ;;
            code)
                if language_is_supported "$extension"
                then
                    echo "$content" | bat --color always --paging never --squeeze-blank --language "$extension" --style "grid,numbers"
                else
                    echo "$content" | bat --color always --paging never --squeeze-blank --style "grid,numbers"
                fi
                ;;
            *) echo -e "\e[1;31mInvalid block type and extension $type / $extension" ;;
        esac
    done < <(psql -U flashback -d flashback -c "select position, type, extension from get_blocks($card) order by position" -At)
}

show_topic_card()
{
    topic_counter=$((topic_counter + 1))

    IFS="|" read -r state position headline < <(psql -U flashback -d flashback -c "select c.state, t.position, c.headline from topics_cards t join cards c on c.id = t.card where t.subject = $subject and t.topic = $topic and t.card = $card order by t.position" -At)

    headline="$(pandoc -f markdown -t plain <<< "$headline" | xargs)"

    case "$mode" in
        aggressive) mode_color="\e[1;31m";;
        progressive) mode_color="\e[1;32m";;
        selective) mode_color="\e[1;35m";;
    esac

    case "$state" in
        draft) state_color="\e[1;2;31m" ;;
        completed) state_color="\e[1;2;33m" ;;
        review) state_color="\e[1;2;35m" ;;
        approved) state_color="\e[1;2;32m" ;;
        released) state_color="\e[1;2;32m" ;;
        rejected) state_color="\e[1;2;3;31m" ;;
    esac

    clear
    {
        printf "${mode_color}%sly practicing\e[0m \e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d / %d)\e[0m\n\n" "${mode^}" "${roadmaps[$roadmap]}" "$roadmap" "$subject_name" "$subject" "$topic_name" "$topic" "$total_topics"
        printf "\e[1;35m%d/%d\e[0m \e[1;33m%s\e[0m \e[2;37m%s\e[0m $state_color(%s)\e[0m\n" "$position" "$total_cards" "$headline" "$card" "$state"

        show_blocks "$card"
    } | custom_pager
}

show_section_card()
{
    section_counter=$((section_counter + 1))

    IFS="|" read -r state position headline < <(psql -U flashback -d flashback -c "select c.state, t.position, c.headline from sections_cards t join cards c on c.id = t.card where t.resource = $resource and t.section = $section and t.card = $card order by position" -At)

    headline="$(pandoc -f markdown -t plain <<< "$headline" | xargs)"

    case "$state" in
        draft) state_color="\e[1;2;31m" ;;
        completed) state_color="\e[1;2;33m" ;;
        review) state_color="\e[1;2;35m" ;;
        approved) state_color="\e[1;2;32m" ;;
        released) state_color="\e[1;2;32m" ;;
        rejected) state_color="\e[1;2;3;31m" ;;
    esac

    clear
    {
        printf "\e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d %s \e[2;32m%s \e[2;31m%s\e[2;37m)\e[0m \e[1;35m»\e[0m \e[1;36m%s / %d\e[0m \e[2;37mpresented by\e[0m \e[1;36m%s\e[0m \e[2;37mprovided by\e[0m \e[1;36m%s\e[0m\n\n" "${roadmaps[$roadmap]}" "$roadmap" "${subjects[$subject]}" "$subject" "${resource_name}" "${resource}" "${resource_type}" "${production}" "${expiration}" "$section_name" ${#sections[*]} "${author}" "${publisher}"
        printf "\e[1;35m%d/%d\e[0m \e[1;33m%s\e[0m \e[2;37m%s\e[0m $state_color(%s)\e[0m\n" "$position" "${#cards[*]}" "$headline" "$card" "$state"

        show_blocks "$card"
    } | custom_pager
}

get_time_difference()
{
    local last_time="$1"

    if [ -n "$last_time" ]
    then
        days="$((($(date +%s) - $(date +%s -d "$last_time")) / 86400))"
        echo -e "\e[1;35mlast study $days days ago\e[0m"
    else
        echo -e "\e[2;35mno progress\e[0m"
    fi
}

get_credits()
{
    local provider="$1"
    local presenter="$2"

    if [ -z "$provider" ] && [ -z "$presenter" ]
    then
        printf "\e[2;37m(unknown source)\e[0m"
    elif [ -n "$provider" ] && [ -z "$presenter" ]
    then
        printf "\e[2;37m(from \e[0;1;34m%s\e[0m\e[0;2;37m)\e[0m" "$provider"
    elif [ -n "$presenter" ] && [ -z "$provider" ]
    then
        printf "\e[2;37m(by \e[0;1;34m%s\e[0;2;37m)\e[0m" "$presenter"
    else
        printf "\e[2;37m(by \e[0;1;34m%s\e[0m \e[2;37mfrom \e[0;1;34m%s\e[0;2;37m)\e[0m" "$presenter" "$provider"
    fi
}

load_resources()
{
    local roadmap="$1"
    local subject="$2"
    declare -A resources
    local position
    local name
    local level

    while IFS='|' read -r id name
    do
        resources[$id]="$name"
    done < <(psql -U flashback -d flashback -c "select id, name from get_resources($user, $subject)" -At)

    clear
    {
        printf "\e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37mresources (%d)\e[0m\n\n" "${roadmaps[$roadmap]}" "$roadmap" "${subjects[$subject]}" "$subject"
        {
            while IFS='|' read -r id name type production expiration presenter provider last_read
            do
                printf "\e[1;35m%5d\e[0m \e[1;33m%s\e[0m \e[2;37m(%s)\e[0m %s %s \e[2;37m(\e[2;32m%s \e[2;31m%s\e[2;37m)\e[0m\n" "$id" "$name" "$type" "$(get_time_difference "$last_read")" "$(get_credits "$provider" "$presenter")" "$production" "$expiration"
            done < <(psql -U flashback -d flashback -c "select id, name, type, production, expiration, presenter, provider, last_read from get_resources($user, $subject) order by name" -At)
        } | dense_column
    } | custom_pager

    while true
    do
        echo
        read -r -p "Select a resource to study, type [t] to switch to topics, or type [p] to begin practicing ${subjects[$subject]}, or [P] to start over: " resource

        if [ "${resource}" == "p" ]
        then
            practice_subject 1
            break
        elif [ "${resource}" == "P" ]
        then
            practice_subject
            break
        elif [ "${resource,,}" == "t" ]
        then
            load_topics "$subject"
            break
        elif [ -n "${resources[$resource]}" ]
        then
            start_study "$resource" "$subject"
            break
        fi
    done
}

load_topics()
{
    subject="$1"
    declare -A topics
    declare -A levels

    while IFS='|' read -r id level
    do
        levels[$id]="$level"
    done < <(psql -U flashback -d flashback -c "select subject, level from milestones where roadmap = $roadmap and subject = $subject" -At)

    level=${levels[$subject]}

    while IFS='|' read -r position name
    do
        topics[$position]="$name"
    done < <(psql -U flashback -d flashback -c "select position, name from get_topics($subject, '$level'::expertise_level)" -At)

    clear
    {
        printf "\e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d)\e[0m\n\n" "${roadmaps[$roadmap]}" "$roadmap" "${subjects[$subject]}" "$subject"
        list_topics
    } | custom_pager

    while true
    do
        echo
        read -r -p "Select a topic, type [r] to switch to ${subjects[$subject]} resources, or type [p] to practice ${subjects[$subject]}, or [P] to start over: " topic

        if [ "${topic,,}" == "r" ]
        then
            load_resources "$roadmap" "$subject"
            break
        elif [ "${topic}" == "p" ]
        then
            practice_subject 1
            break
        elif [ "${topic}" == "P" ]
        then
            practice_subject
            break
        elif [ -n "${topics[$topic]}" ]
        then
            practice_topic "$roadmap" "$milestone" "$topic"
            break
        fi
    done
}

load_roadmaps()
{
    declare -A roadmaps
    declare -A milestones
    local roadmap
    local id
    local name
    local subject
    local response

    while IFS='|' read -r id name
    do
        roadmaps[$id]="$name"
    done < <(psql -U flashback -d flashback -c "select id, name from get_roadmaps($user) order by id" -At)

    clear
    {
        printf "\e[1;36m%s\e[0m\n\n" "Flashback"
        {
            while IFS='|' read -r id name
            do
                printf "\e[1;35m%5d\e[0m \e[1;33m%s\e[0m\n" "$id" "$name"
            done < <(psql -U flashback -d flashback -c "select id, name from get_roadmaps($user) order by id" -At)
        } | dense_column
    } | custom_pager

    while true
    do
        echo
        read -r -p "Select a roadmap: " roadmap
        [[ -n "${roadmaps[$roadmap]}" ]] && break
    done

    while IFS='|' read -r position id name
    do
        milestones[$position]="$id"
    done < <(psql -U flashback -d flashback -c "select position, subject, level from milestones where roadmap = $roadmap" -At)

    while IFS='|' read -r id name
    do
        subjects[id]="$name"
    done < <(psql -U flashback -d flashback -c "select s.id, s.name from subjects s join milestones m on m.subject = s.id where m.roadmap = $roadmap" -At)

    clear
    {
        printf "\e[1;36m%s\e[0m \e[2;37m(%d)\e[0m\n\n" "${roadmaps[$roadmap]}" "$roadmap"
        {
            while IFS='|' read -r position id name level
            do
                case "$level" in
                    surface) level_symbol="$(echo -e "\e[1;32m\u2a\u01\u01\e[0m")" ;;
                    depth) level_symbol="$(echo -e "\e[1;33m\u2051\e[0m")" ;;
                    origin) level_symbol="$(echo -e "\e[1;31m\u2042\u01\e[0m")" ;;
                esac
                printf "\e[1;35m%5d\e[0m \e[1;33m%s\e[0m \e[2;37m%s\e[0m\n" "$position" "$name" "$level_symbol"
            done < <(psql -U flashback -d flashback -c "select position, id, name, level from get_subjects($roadmap) order by position" -At)
        } | dense_column
    } | custom_pager

    while true
    do
        echo
        read -r -p "Select a subject number or type [p] to begin practicing all subjects: " milestone

        #if [ "${milestone,,}" == "p" ]
        #then
        #    start_practice "$roadmap"
        #    break
        if [ -n "${milestones[$milestone]}" ]
        then
            subject=${milestones[$milestone]}
            load_topics "$subject"
            break
        fi
    done
}

practice_subject()
{
    local start_over="$1"
    total_topics="$(psql -At -U flashback -d flashback -c "select count(*) from topics where subject = $subject")"

    while IFS="|" read -r topic level
    do
        unset cards
        unset assessment
        topic_counter=0

        subject_name="$(psql -U flashback -d flashback -c "select name from subjects where id = $subject" -At)"
        topic_name="$(psql -U flashback -d flashback -c "select name from topics where subject = $subject and position = $topic" -At)"

        if [ ${start_over:-0} -eq 1 ]
        then
            mode="$(psql -U flashback -d flashback -c "select * from get_practice_mode($user, $subject, '$level'::expertise_level)" -At)"
        else
            filter="where now() - last_practice > '1 day'"
            mode="selective"
        fi

        assessment="$(psql -At -U flashback -d flashback -c "select assessment from get_assessments($user, $subject, $topic, '$level'::expertise_level) where assimilations >= 3 order by level desc, assimilations desc limit 1;")"

        if [ -n "$assessment" ]
        then
            cards=( $assessment )
            total_cards=1
        else
            readarray -t cards < <(psql -At -U flashback -d flashback -c "select card from get_practice_cards($user, $subject, '$level'::expertise_level) where topic = $topic and level = '$level'::expertise_level order by position")
            total_cards="$(psql -At -U flashback -d flashback -c "select count(*) from topics_cards where subject = $subject and topic = $topic")"
        fi

        card_index=0

        while [ ${#cards[*]} -gt 0 ]
        do
            card="${cards[$card_index]}"
            starting_time=$(date +%s)

            show_topic_card

            available_keys=( "n" )
            available_keys+=( "move forward" )

            if [ $card_index -gt 0 ]
            then
                available_keys+=( "p" )
                available_keys+=( "move backward" )
            fi

            if [ "$state" == "draft" ]
            then
                available_keys+=( "R" )
                available_keys+=( "assign card to a resource" )
            elif [ "$state" == "review" ]
            then
                available_keys+=( "M" )
                available_keys+=( "move card to another topic" )
            fi

            while true
            do
                get_key "${available_keys[@]}"

                if ! slip_guard; then continue; fi

                case "$key" in
                    R) move_card_to_section; break ;;
                    M) move_card_to_topic; card_index=$((card_index - 1)); break ;;
                    n) card_index=$((card_index + 1)); break ;;
                    p) card_index=$((card_index - 1)); break ;;
                esac
            done

            make_progress

            [ $card_index -ge ${#cards[*]} ] && break; 
        done
    done < <(psql -U flashback -d flashback -c "select topic, level from get_practice_cards($user, $subject, '$level'::expertise_level) ${filter} group by topic, level order by topic" -At)
    echo
}

practice_topic()
{
    total_topics="$(psql -At -U flashback -d flashback -c "select count(*) from topics where subject = $subject")"

    while IFS="|" read -r topic level
    do
        unset cards
        unset assessment
        topic_counter=0

        subject_name="$(psql -U flashback -d flashback -c "select name from subjects where id = $subject" -At)"
        topic_name="$(psql -U flashback -d flashback -c "select name from topics where subject = $subject and position = $topic" -At)"
        mode="selective"

        readarray -t cards < <(psql -At -U flashback -d flashback -c "select card from get_topics_cards($roadmap, $subject, $topic) order by position")
        total_cards="$(psql -At -U flashback -d flashback -c "select count(*) from topics_cards where subject = $subject and topic = $topic")"

        card_index=0

        while [ ${#cards[*]} -gt 0 ]
        do
            card="${cards[$card_index]}"
            starting_time=$(date +%s)

            show_topic_card

            available_keys=( "n" )
            available_keys+=( "move forward" )

            if [ $card_index -gt 0 ]
            then
                available_keys+=( "p" )
                available_keys+=( "move backward" )
            fi

            if [ "$state" == "draft" ]
            then
                available_keys+=( "R" )
                available_keys+=( "assign card to a resource" )
            elif [ "$state" == "review" ]
            then
                available_keys+=( "M" )
                available_keys+=( "move card to another topic" )
            fi

            while true
            do
                get_key "${available_keys[@]}"

                if ! slip_guard; then continue; fi

                case "$key" in
                    R) move_card_to_section; break ;;
                    M) move_card_to_topic; card_index=$((card_index - 1)); break ;;
                    n) card_index=$((card_index + 1)); break ;;
                    p) card_index=$((card_index - 1)); break ;;
                esac
            done

            make_progress

            [ $card_index -ge ${#cards[*]} ] && break; 
        done
    done < <(psql -U flashback -d flashback -c "select position, level from topics where subject = $subject and position = $topic order by position" -At)
    echo
}

prompt()
{
    legend="$1"
    tput cup $last_line $(($(tput cols) - ${#legend}))
    tput civis
    echo -ne "\e[1;31m$legend\e[0m"
}

wipe()
{
    for line_number in $(seq "$1" "$2")
    do
        tput cup "$line_number" 0
        printf "%*.s " $((COLUMNS - 1)) 0
    done
}

list_topics()
{
    local selection="$1"
    local selection_color="\e[1;33m"

    while IFS="|" read -r position name pattern
    do
        pattern="${pattern^}"
        name="${name:-$pattern $position}"

        if [ -n "$selection" ] && [ "$selection" -eq "$position" ]
        then
            selection_color="\e[1;35m"
        elif [ -n "$selection" ] && [[ "$position" =~ ^$selection ]]
        then
            selection_color="\e[1;31m"
        else
            selection_color="\e[1;33m"
        fi

        printf "\e[1;34m%d\e[0m $selection_color%s\e[0m\n" "$position" "${name}"
    done < <(psql -U flashback -d flashback -c "select t.position, t.name from topics t where t.subject = $subject order by t.position" -At) | dense_column

    capture_line
}

list_resources()
{
    local selection="$1"
    local selection_color="\e[1;33m"

    while IFS="|" read -r resource, name, type, production, expiration, presenter, provider, last_read
    do
        pattern="${pattern^}"
        name="${name:-$pattern $position}"

        if [ -n "$selection" ] && [ "$selection" -eq "$position" ]
        then
            selection_color="\e[1;35m"
        elif [ -n "$selection" ] && [[ "$position" =~ ^$selection ]]
        then
            selection_color="\e[1;31m"
        else
            selection_color="\e[1;33m"
        fi

        printf "\e[1;34m%d\e[0m $selection_color%s\e[0m\n" "$position" "${name}"
    done < <(psql -U flashback -d flashback -c "select id, name, type, production, expiration, presenter, provider, last_read from get_resources(1, $subject)" -At)

    capture_line
}

list_sections()
{
    local selection="$1"
    local selection_color="\e[1;33m"

    while IFS="|" read -r position name pattern
    do
        pattern="${pattern^}"
        name="${name:-$pattern $position}"

        if [ -n "$selection" ] && [ "$selection" -eq "$position" ]
        then
            selection_color="\e[1;35m"
        elif [ -n "$selection" ] && [[ "$position" =~ ^$selection ]]
        then
            selection_color="\e[1;31m"
        else
            selection_color="\e[1;33m"
        fi

        printf "\e[1;34m%d\e[0m $selection_color%s\e[0m\n" "$position" "${name}"
    done < <(psql -U flashback -d flashback -c "select s.position, s.name, r.pattern from sections s join resources r on r.id = s.resource where s.resource = $resource order by s.position" -At) | dense_column | custom_pager

    capture_line
}

create_topic()
{
    tput cup "$current_line" 0
    tput cnorm
    read -r -p "Topic name: " topic_name </dev/tty

    echo -ne "Does \e[1;33m$topic_name\e[0m have a specific location? "

    while true
    do
        read -n1 -r response </dev/tty
        [ "${response,,}" == "y" ] && break
        [ "${response,,}" == "n" ] && break
    done

    if [ "${response,,}" == "y" ]
    then
        tput cnorm
        echo -ne "\nWhat is the position of this topic? "
        read -r topic </dev/tty
        tput civis

        echo -ne "Topic \e[1;33m$topic_name\e[0m in position \e[1;33m${topic}\e[0m of subject \e[1;33m${subjects[$subject]}\e[0m will be created, correct? "
        while true
        do
            read -n1 -r response </dev/tty
            [ "${response,,}" == "y" ] && break
            [ "${response,,}" == "n" ] && return
        done

        psql -U flashback -d flashback -c "call create_topic($subject, 'surface', '$topic_name', $topic)"
    else
        echo -ne "\nTopic \e[1;33m$topic_name\e[0m in subject \e[1;33m${subjects[$subject]}\e[0m will be created, correct? "
        while true
        do
            read -n1 -r response </dev/tty
            [ "${response,,}" == "y" ] && break
            [ "${response,,}" == "n" ] && return
        done

        topic="$(psql -U flashback -d flashback -c "select * from create_topic($subject, 'surface', '$topic_name')" -At)"
    fi

    tput civis

    topics[$topic]="$topic_name"
}

create_section()
{
    tput cup "$current_line" 0
    tput cnorm
    read -r -p "Section name: " section_name </dev/tty

    echo -ne "Does \e[1;33m$section_name\e[0m have a specific location? "

    while true
    do
        read -n1 -r response </dev/tty
        [ "${response,,}" == "y" ] && break
        [ "${response,,}" == "n" ] && break
    done

    if [ "${response,,}" == "y" ]
    then
        tput cnorm
        echo -ne "\nWhat is the position of this section? "
        read -r section </dev/tty
        tput civis

        echo -ne "Section \e[1;33m$section_name\e[0m in position \e[1;33m${section}\e[0m of resource \e[1;33m${resources[$resource]} \e[0;2;38m($resource)\e[0m will be created, correct? "
        while true
        do
            read -n1 -r response </dev/tty
            [ "${response,,}" == "y" ] && break
            [ "${response,,}" == "n" ] && return
        done

        psql -U flashback -d flashback -c "call create_section($resource, '$section_name', $section)"
    else
        echo -ne "\nSection \e[1;33m$section_name\e[0m in resource \e[1;33m${resources[$resource]} \e[0;2;38m($resource)\e[0m will be created, correct? "
        while true
        do
            read -n1 -r response </dev/tty
            [ "${response,,}" == "y" ] && break
            [ "${response,,}" == "n" ] && return
        done

        section="$(psql -U flashback -d flashback -c "select * from create_section($resource, '$section_name')" -At)"
    fi

    tput civis

    sections[$section]="$section_name"
}

assign_card_to_topic()
{
    local card="$1"
    local subject="$2"
    local topic
    local level
    local position
    local name
    local response
    local -A topics

    while IFS="|" read -r position name
    do
        topics[$position]="$name"
    done < <(psql -U flashback -d flashback -c "select t.position, t.name from topics t where t.subject = $subject order by t.position" -At)

    tput civis

    topic=""
    while true
    do
        wipe 4 $last_line
        tput cup 5 0
        echo -e "Available topics in \e[1;33m${subjects[$subject]}\e[0m:\n"
        list_topics "$topic"

        tput civis
        prompt "Select a topic from the list, or press T to create a new topic"

        read -r -n1 -s response </dev/tty
        echo

        case "$response" in
            T) [ -z "$topic" ] && create_topic ;;
            $'\x7f') [ -z "${topic}" ] && return; topic="${topic:0:-1}" ;;
            *[!0-9]*) topic="" ;;
            *) [ ${#response} -eq 0 ] && if [ -n "${topics[$topic]}" ]; then break; else topic=""; fi; topic+="$response" ;;
        esac
    done

    tput civis
    tput cup "$current_line" 0
    echo -ne "The card will be added to \e[1;33m${topics[$topic]}\e[0m in \e[1;33m${subjects[$subject]}\e[0m, is this correct?\n"

    while true
    do
        read -n1 -r response </dev/tty
        [ "${response,,}" == "y" ] && break
        [ "${response,,}" == "n" ] && return
    done

    tput cnorm

    if ! position=$(psql -U flashback -d flashback -c "select * from add_card_to_topic($subject, 'surface', $topic, $card)" -At)
    then
        echo -e "\e[1;31mFailed to assign card $card to subject $subject in position $topic"
        exit 1
    fi
}

move_card_to_topic()
{
    local previous_topic="$topic"

    unset topics
    declare -A topics

    while IFS="|" read -r position name
    do
        topics[$position]="$name"
    done < <(psql -U flashback -d flashback -c "select t.position, t.name from topics t where t.subject = $subject order by t.position" -At)

    tput civis

    topic=""
    while true
    do
        wipe 4 $last_line
        tput cup 5 0
        echo -e "Available topics in \e[1;33m${subjects[$subject]}\e[0m:\n"
        list_topics "$topic"

        tput civis
        prompt "Select a topic from the list, or press T to create a new topic"

        read -r -n1 -s response </dev/tty
        echo

        case "$response" in
            T) [ -z "$topic" ] && create_topic ;;
            $'\x7f') [ -z "${topic}" ] && return; topic="${topic:0:-1}" ;;
            *[!0-9]*) topic="" ;;
            *) [ ${#response} -eq 0 ] && if [ -n "${topics[$topic]}" ]; then break; else topic=""; fi; topic+="$response" ;;
        esac
    done

    tput civis
    tput cup "$current_line" 0
    echo -ne "The card will be moved to \e[1;33m${topics[$topic]}\e[0m in \e[1;33m${subjects[$subject]}\e[0m, is this correct?"

    while true
    do
        read -n1 -r response </dev/tty
        [ "${response,,}" == "y" ] && break
        [ "${response,,}" == "n" ] && return
    done

    local current_topic
    current_topic="$(psql -U flashback -d flashback -c "select topic from topics_cards where subject = $subject and level = 'surface' and card = $card" -At)"

    tput cnorm

    if [ -n "$current_topic" ]
    then
        if ! position="$(psql -U flashback -d flashback -c "select * from move_card_to_topic($card, $subject, 'surface', $current_topic, $topic)" -At)"
        then
            echo -e "\e[1;31mFailed to move card $card from topic ${topics[$current_topic]} ($current_topic) to topic ${topics[$topic]} ($topic) in subject ${subjects[$subject]} ($subject)"
            exit 1
        fi
    else
        if ! position="$(psql -U flashback -d flashback -c "select * from add_card_to_topic($subject, 'surface', $topic, $card)" -At)"
        then
            echo -e "\e[1;31mFailed to add card $card to subject ${subjects[$subject]} ($subject) in topic ${topics[$topic]} ($topic)"
            exit 1
        fi
    fi

    topic="$previous_topic"

    unset cards
    readarray -t cards < <(psql -At -U flashback -d flashback -c "select c.id from topics_cards t join cards c on c.id = t.card where t.subject = $subject and t.level = 'surface' and t.topic = $topic order by t.position")
}

move_card_to_section()
{
    local previous_resource="$resource"
    local previous_section="$section"

    local selected_resource="$1"

    unset resources
    declare -A resources
    unset sections
    declare -A sections

    if [ -z "$selected_resource" ]
    then
        tput civis

        resource=""
        while true
        do
            wipe 4 $last_line
            tput cup 5 0
            echo -e "Available resources in \e[1;33m${subjects[$subject]} ($subject)\e[0m:\n"
            list_resources "$resource"

            tput civis
            prompt "Select a resource from the list"

            read -r -n1 -s response </dev/tty
            echo

            case "$response" in
                #R) [ -z "$resource" ] && create_resource ;;
                $'\x7f') [ -z "${resource}" ] && return; resource="${resource:0:-1}" ;;
                *[!0-9]*) resource="" ;;
                *) [ ${#response} -eq 0 ] && if [ -n "${resources[$resource]}" ]; then break; else resource=""; fi; resource+="$response" ;;
            esac
        done
    else
        resource="$selected_resource"
    fi

    while IFS="|" read -r position name pattern
    do
        pattern="${pattern^}"
        name="${name:-$pattern $position}"
        sections[$position]="$name"
    done < <(psql -U flashback -d flashback -c "select s.position, s.name, r.pattern from sections s join resources r on r.id = s.resource where s.resource = $resource order by s.position" -At)

    tput civis

    section=""
    while true
    do
        wipe 4 $last_line
        tput cup 5 0
        echo -e "Available sections in \e[1;33m${resources[$resource]}\e[0m:\n"
        list_sections "$section"

        tput civis
        prompt "Select a section from the list, or press S to create a new section"

        read -r -n1 -s response </dev/tty
        echo

        case "$response" in
            S) [ -z "$section" ] && create_section ;;
            $'\x7f') [ -z "${section}" ] && return; section="${section:0:-1}" ;;
            *[!0-9]*) section="" ;;
            *) [ ${#response} -eq 0 ] && if [ -n "${sections[$section]}" ]; then break; else section=""; fi; section+="$response" ;;
        esac
    done

    tput civis
    tput cup "$current_line" 0
    echo -ne "The card will be moved to \e[1;33m${sections[$section]}\e[0m in \e[1;33m${resources[$resource]}\e[0m, is this correct?\n"

    while true
    do
        read -n1 -r response </dev/tty
        [ "${response,,}" == "y" ] && break
        [ "${response,,}" == "n" ] && return
    done

    local current_section
    current_section="$(psql -U flashback -d flashback -c "select section from sections_cards where resource = $resource and card = $card" -At)"

    tput cnorm

    if [ -n "$current_section" ]
    then
        if ! position="$(psql -U flashback -d flashback -c "select * from move_card_to_section($card, $resource, $current_section, $section)" -At)"
        then
            echo -e "\e[1;31mFailed to move card $card to resource ${resources[$resource]} ($resource) in section ${sections[$section]} ($section)"
            exit 1
        fi
    else
        if ! position=$(psql -U flashback -d flashback -c "select * from add_card_to_section($resource, $section, $card)" -At)
        then
            echo -e "\e[1;31mFailed to move card $card to resource ${resources[$resource]} ($resource) in section ${sections[$section]} ($section)"
            exit 1
        fi
    fi

    [ -n "$previous_resource" ] && resource="$previous_resource"
    [ -n "$previous_section" ] && section="$previous_section"

    unset cards
    readarray -t cards < <(psql -At -U flashback -d flashback -c "select c.id from sections_cards t join cards c on c.id = t.card where resource = $resource and section = $section order by position")
}

slip_guard()
{
    local lapse

    lapse=$(date +%s)
    if [ $((lapse - starting_time)) -lt 3 ]
    then
        return 1
    fi
}

make_progress()
{
    ending_time=$(date +%s)
    duration=$((ending_time - starting_time))

    psql -U flashback -d flashback -c "call make_progress($user, $card, $duration, '$mode'::practice_mode)" -At;
}

start_study()
{
    local resource="$1"
    local subject="$2"
    local -A sections
    local -a available_keys
    local -a cards
    mode="selective"

    while IFS="|" read -r position name pattern
    do
        sections[$position]="${pattern^} ${name:-$position}"
    done < <(psql -U flashback -d flashback -c "select s.position, s.name, r.pattern from resources r join sections s on s.resource = r.id where r.id = $resource order by s.position" -At)

    clear
    {
        printf "\e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d)\e[0m \e[1;35m»\e[0m \e[1;36m%s\e[0m \e[2;37m(%d)\e[0m\n\n" "${roadmaps[$roadmap]}" "$roadmap" "${subjects[$subject]}" "$subject" "${resources[$resource]}" "$resource"
        {
            while IFS="|" read -r position name pattern total exported
            do
                pattern="${pattern^}"
                name="${name:-$pattern $position}"
                printf "\e[1;35m%5d\e[0m \e[1;33m%s\e[0m \e[2;28m(%d/%d)\e[0m\n" "$position" "$name" "$exported" "$total"
            done < <(psql -U flashback -d flashback -c "select s.position, s.name, r.pattern, count(sc.card), count(tc.card) from resources r join sections s on r.id = s.resource left join sections_cards sc on sc.section = s.position and sc.resource = s.resource left join topics_cards tc on tc.card = sc.card where s.resource = $resource group by s.position, s.name, r.pattern order by s.position;" -At)
        } | dense_column
    } | custom_pager

    local selected_sections=()

    while true
    do
        echo
        link="$(psql -U flashback -d flashback -c "select link from resources where id = $resource" -At)"
        read -r -p "Select a section or type all to study all sections${link+:" or type open to visit resource link"}: " section
        { [ -n "${sections[$section]}" ] || [ "$section" == "all" ]; } && break
        [ -n "$link" ] && [ "$section" == "open" ] && xdg-open "$link" &>/dev/null &
    done
    echo

    if [ "$section" == "all" ]
    then
        selected_sections=( "${!sections[@]}" )
    else
        selected_sections=( "$section" )
    fi

    [ ${#selected_sections[*]} -eq 0 ] && return

    while IFS="|" read -r resource resource_name resource_type pattern production expiration author publisher link last_read
    do
        while IFS="|" read -r section section_name
        do
            section_counter=0

            if [ -n "$section_name" ]
            then
                section_name="${section_name} ${section}"
            else
                section_name="${pattern^} ${section}"
            fi

            unset cards
            readarray -t cards < <(psql -At -U flashback -d flashback -c "select c.id from sections_cards t join cards c on c.id = t.card where resource = $resource and section = $section order by position")

            card_index=0

            while [ ${#cards[*]} -gt 0 ]
            do
                card="${cards[$card_index]}"
                starting_time=$(date +%s)

                show_section_card

                available_keys=( "n" )
                available_keys+=( "move forward" )

                if [ $card_index -gt 0 ]
                then
                    available_keys+=( "p" )
                    available_keys+=( "move backward" )
                fi

                if [ "$state" == "draft" ]
                then
                    available_keys+=( "A" )
                    available_keys+=( "assign card to a topic" )
                elif [ "$state" == "review" ]
                then
                    available_keys+=( "M" )
                    available_keys+=( "move card to another section" )
                fi

                while true
                do
                    get_key "${available_keys[@]}"

                    if ! slip_guard; then continue; fi

                    case "$key" in
                        A) assign_card_to_topic "$card" "$subject"; break ;;
                        M) move_card_to_section "$resource"; unset cards[$card]; card_index=$((card_index - 1)); break ;;
                        n) card_index=$((card_index + 1)); break ;;
                        p) card_index=$((card_index - 1)); break ;;
                    esac
                done

                make_progress

                [ $card_index -ge ${#cards[*]} ] && break; 
            done

        done < <(psql -U flashback -d flashback -c "select position, name from sections where resource = $resource and position in ( $(tr ' ' ',' <<< "${selected_sections[*]}") ) order by position" -At)
    done < <(psql -U flashback -d flashback -c "select id, name, type, pattern, production, expiration, presenter, provider, link, last_read from get_resources($user, $subject) where id = $resource order by name" -At)
    echo
}

load_roadmaps
