--
-- PostgreSQL database dump
--

\restrict dGsHx9ipa2gODb2S9cKl6h2eqF1cIWxNxqwFxazD2Qp4zcIwKBbbxjjQBrh6Alf

-- Dumped from database version 18.0
-- Dumped by pg_dump version 18.0

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: flashback; Type: SCHEMA; Schema: -; Owner: flashback
--

CREATE SCHEMA flashback;


ALTER SCHEMA flashback OWNER TO flashback;

--
-- Name: card_state; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.card_state AS ENUM (
    'draft',
    'completed',
    'review',
    'approved',
    'released',
    'rejected'
);


ALTER TYPE flashback.card_state OWNER TO flashback;

--
-- Name: condition; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.condition AS ENUM (
    'draft',
    'relevant',
    'outdated',
    'canonical',
    'abandoned'
);


ALTER TYPE flashback.condition OWNER TO flashback;

--
-- Name: content_type; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.content_type AS ENUM (
    'text',
    'code'
);


ALTER TYPE flashback.content_type OWNER TO flashback;

--
-- Name: expertise_level; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.expertise_level AS ENUM (
    'surface',
    'depth',
    'origin'
);


ALTER TYPE flashback.expertise_level OWNER TO flashback;

--
-- Name: network_activity; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.network_activity AS ENUM (
    'signup',
    'signin',
    'signout',
    'upload',
    'download'
);


ALTER TYPE flashback.network_activity OWNER TO flashback;

--
-- Name: practice_mode; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.practice_mode AS ENUM (
    'aggressive',
    'progressive',
    'selective'
);


ALTER TYPE flashback.practice_mode OWNER TO flashback;

--
-- Name: resource_type; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.resource_type AS ENUM (
    'book',
    'website',
    'course',
    'video',
    'channel',
    'mailing list',
    'manual',
    'slides',
    'user'
);


ALTER TYPE flashback.resource_type OWNER TO flashback;

--
-- Name: section_pattern; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.section_pattern AS ENUM (
    'chapter',
    'page',
    'session',
    'episode',
    'post',
    'synapse'
);


ALTER TYPE flashback.section_pattern OWNER TO flashback;

--
-- Name: user_action; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.user_action AS ENUM (
    'creation',
    'modification',
    'deletion',
    'joining',
    'spliting',
    'reordering',
    'moving'
);


ALTER TYPE flashback.user_action OWNER TO flashback;

--
-- Name: user_state; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.user_state AS ENUM (
    'active',
    'inactive',
    'suspended',
    'banned'
);


ALTER TYPE flashback.user_state OWNER TO flashback;

--
-- Name: add_card_to_assessments(integer, flashback.expertise_level, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_card_to_assessments(IN subject integer, IN level flashback.expertise_level, IN topic integer, IN card integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into assessments (subject, level, topic, card)
    values (add_card_to_assessments.subject, add_card_to_assessments.level, add_card_to_assessments.topic, add_card_to_assessments.card);

    update cards set state = 'review'::card_state where cards.id = add_card_to_assessments.card;
end;
$$;


ALTER PROCEDURE flashback.add_card_to_assessments(IN subject integer, IN level flashback.expertise_level, IN topic integer, IN card integer) OWNER TO flashback;

--
-- Name: add_card_to_section(integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.add_card_to_section(resource integer, section integer, card integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(position), 0) + 1 into top_position from sections_cards sc where sc.resource = add_card_to_section.resource and sc.section = add_card_to_section.section;
    insert into sections_cards (resource, section, card, position) values (resource, section, card, top_position);
    update cards set state = 'completed'::card_state where id = card;
    return top_position;
end;
$$;


ALTER FUNCTION flashback.add_card_to_section(resource integer, section integer, card integer) OWNER TO flashback;

--
-- Name: add_card_to_section(integer, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_card_to_section(IN resource integer, IN section integer, IN card integer, IN "position" integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into sections_cards (resource, section, card, position) values (resource, section, card, position);
    update cards set state = 'completed'::card_state where id = card;
end;
$$;


ALTER PROCEDURE flashback.add_card_to_section(IN resource integer, IN section integer, IN card integer, IN "position" integer) OWNER TO flashback;

--
-- Name: add_card_to_topic(integer, flashback.expertise_level, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.add_card_to_topic(subject integer, level flashback.expertise_level, topic integer, card integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(position), 0) + 1 into top_position
    from topics_cards tc
    where tc.subject = add_card_to_topic.subject and tc.level = add_card_to_topic.level and tc.topic = add_card_to_topic.topic;

    insert into topics_cards (subject, level, topic, card, position)
    values (add_card_to_topic.subject, add_card_to_topic.level, add_card_to_topic.topic, add_card_to_topic.card, top_position);

    update cards set state = 'review'::card_state where cards.id = add_card_to_topic.card;

    return top_position;
end;
$$;


ALTER FUNCTION flashback.add_card_to_topic(subject integer, level flashback.expertise_level, topic integer, card integer) OWNER TO flashback;

--
-- Name: add_card_to_topic(integer, flashback.expertise_level, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_card_to_topic(IN subject integer, IN level flashback.expertise_level, IN topic integer, IN card integer, IN "position" integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into topics_cards (subject, level, topic, card, position)
    values (add_card_to_topic.subject, add_card_to_topic.level, add_card_to_topic.topic, add_card_to_topic.card, add_card_to_topic.position);

    update cards set state = 'review'::card_state where cards.id = add_card_to_topic.card;
end;
$$;


ALTER PROCEDURE flashback.add_card_to_topic(IN subject integer, IN level flashback.expertise_level, IN topic integer, IN card integer, IN "position" integer) OWNER TO flashback;

--
-- Name: add_resource_to_subject(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_resource_to_subject(IN resource_id integer, IN subject_id integer)
    LANGUAGE plpgsql
    AS $$ begin insert into shelves (resource, subject) values (resource_id, subject_id); end; $$;


ALTER PROCEDURE flashback.add_resource_to_subject(IN resource_id integer, IN subject_id integer) OWNER TO flashback;

--
-- Name: add_subject_to_roadmap(integer, flashback.expertise_level, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.add_subject_to_roadmap(subject integer, level flashback.expertise_level, roadmap integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(milestones.position), 0) + 1 into top_position from milestones where milestones.roadmap = add_subject_to_roadmap.roadmap;

    insert into milestones(roadmap, subject, level, position) values (add_subject_to_roadmap.roadmap, add_subject_to_roadmap.subject, add_subject_to_roadmap.level, top_position);

    return top_position;
end;
$$;


ALTER FUNCTION flashback.add_subject_to_roadmap(subject integer, level flashback.expertise_level, roadmap integer) OWNER TO flashback;

--
-- Name: add_subject_to_roadmap(integer, flashback.expertise_level, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_subject_to_roadmap(IN subject integer, IN level flashback.expertise_level, IN roadmap integer, IN "position" integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(milestones.position), 0) + 1 into top_position from milestones where milestones.roadmap = add_subject_to_roadmap.roadmap;

    update milestones m set position = m.position + top_position where m.roadmap = add_subject_to_roadmap.roadmap and m.position >= add_subject_to_roadmap.position;

    insert into milestones(roadmap, subject, level, position) values (
        add_subject_to_roadmap.roadmap, add_subject_to_roadmap.subject, add_subject_to_roadmap.level, add_subject_to_roadmap.position
    );

    update milestones m set position = s.altered_position
    from (
        select mm.position, mm.level, row_number() over (order by mm.position) as altered_position from milestones mm where mm.roadmap = add_subject_to_roadmap.roadmap
    ) s
    where m.roadmap = add_subject_to_roadmap.roadmap and m.level = s.level and m.position = s.position;
end;
$$;


ALTER PROCEDURE flashback.add_subject_to_roadmap(IN subject integer, IN level flashback.expertise_level, IN roadmap integer, IN "position" integer) OWNER TO flashback;

--
-- Name: change_block_extension(integer, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_block_extension(IN selected_card integer, IN block integer, IN new_extension character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update blocks set extension = new_extension where card = selected_card and position = block;
end;
$$;


ALTER PROCEDURE flashback.change_block_extension(IN selected_card integer, IN block integer, IN new_extension character varying) OWNER TO flashback;

--
-- Name: change_block_type(integer, integer, flashback.content_type); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_block_type(IN selected_card integer, IN block integer, IN new_type flashback.content_type)
    LANGUAGE plpgsql
    AS $$
begin
    update blocks set type = new_type where card = selected_card and position = block;
end;
$$;


ALTER PROCEDURE flashback.change_block_type(IN selected_card integer, IN block integer, IN new_type flashback.content_type) OWNER TO flashback;

--
-- Name: change_users_hash(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_users_hash(IN user_id integer, IN hash character varying)
    LANGUAGE plpgsql
    AS $$ begin update users set hash = change_users_hash.hash where users.id = change_users_hash.user_id; end; $$;


ALTER PROCEDURE flashback.change_users_hash(IN user_id integer, IN hash character varying) OWNER TO flashback;

--
-- Name: create_block(integer, flashback.content_type, character varying, text); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_block(card integer, type flashback.content_type, extension character varying, content text) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare next_position integer;
begin
    select coalesce(max(position), 0) + 1 into next_position from blocks b where b.card = create_block.card;

    insert into blocks (card, position, content, type, extension) values (card, next_position, content, type, extension);

    return next_position;
end;
$$;


ALTER FUNCTION flashback.create_block(card integer, type flashback.content_type, extension character varying, content text) OWNER TO flashback;

--
-- Name: create_block(integer, flashback.content_type, character varying, text, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_block(IN card integer, IN type flashback.content_type, IN extension character varying, IN content text, IN "position" integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into blocks (card, position, content, type, extension) values (card, position, content, type, extension);
end;
$$;


ALTER PROCEDURE flashback.create_block(IN card integer, IN type flashback.content_type, IN extension character varying, IN content text, IN "position" integer) OWNER TO flashback;

--
-- Name: create_card(text); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_card(headline text) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare card integer;
begin
    insert into cards (headline) values (headline) returning id into card;
    return card;
end;
$$;


ALTER FUNCTION flashback.create_card(headline text) OWNER TO flashback;

--
-- Name: create_resource(character varying, flashback.resource_type, flashback.section_pattern, character varying, character varying, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_resource(name character varying, type flashback.resource_type, pattern flashback.section_pattern, author character varying, publisher character varying, link character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare resource integer;
begin
    insert into resources (name, type, pattern, condition, presenter, provider, link)
    values (name, type, pattern, 'relevant'::condition, nullif(author, ''), nullif(publisher, ''), nullif(link, '')) returning id into resource;

    return resource;
end;
$$;


ALTER FUNCTION flashback.create_resource(name character varying, type flashback.resource_type, pattern flashback.section_pattern, author character varying, publisher character varying, link character varying) OWNER TO flashback;

--
-- Name: create_roadmap(integer, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_roadmap(user_id integer, name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare roadmap integer;
begin
    insert into roadmaps(name) values (name) returning id into roadmap;

    insert into users_roadmaps("user", roadmap) values (user_id, roadmap);
    return roadmap;
end;
$$;


ALTER FUNCTION flashback.create_roadmap(user_id integer, name character varying) OWNER TO flashback;

--
-- Name: create_section(integer, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_section(resource integer, name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare last_position integer;
begin
    if not exists (select 1 from resources where id = create_section.resource) then
        raise notice 'Resource does not exist';
    end if;

    select coalesce(max(position), 0) + 1 into last_position from sections where sections.resource = create_section.resource;

    insert into sections (resource, position, name) values (create_section.resource, last_position, nullif(create_section.name, ''));

    return last_position;
end; $$;


ALTER FUNCTION flashback.create_section(resource integer, name character varying) OWNER TO flashback;

--
-- Name: create_section(integer, character varying, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_section(IN resource integer, IN name character varying, IN "position" integer)
    LANGUAGE plpgsql
    AS $$
declare last_position integer;
--declare new_position integer;
--section record;
begin
    if not exists (select 1 from resources where resources.id = create_section.resource) then
        raise notice 'Resource does not exist';
    end if;

    select max(coalesce(position, 0)) into last_position from sections where sections.resource = create_section.resource;

    update sections set position = sections.position + last_position where sections.resource = create_sction.resource and sections.position >= create_section.position;

    --for section in
    --    select s.position from sections s where s.resource = create_section.resource
    --loop
    --    select section.position + last_position into new_position;

    --    update sections set position = new_position where sections.resource = create_sction.resource and sections.position = create_section.position;
    --end loop;


    insert into sections (resource, position, name) values (create_section.resource, create_section.position, nullif(create_section.name, ''));

    --for section_record in
    --    select s.position from sections s where s.resource = create_section.resource
    --loop
    --end loop;

    update sections set position = ss.new_position
    from (
        select row_number() over (order by s.position) as new_position, s.position
        from sections s
        where s.resource = create_section.resource
    ) ss
    where sections.resource = create_section.resource and ss.position = sections.position;
end; $$;


ALTER PROCEDURE flashback.create_section(IN resource integer, IN name character varying, IN "position" integer) OWNER TO flashback;

--
-- Name: create_session(integer, character varying, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_session(IN user_id integer, IN token character varying, IN device character varying)
    LANGUAGE plpgsql
    AS $$
declare previous_token varchar(300);
declare last_use date;
begin
    select s.token, s.last_usage into previous_token, last_use from sessions s where s."user" = user_id and s.device = create_session.device;

    if previous_token is null then
        insert into sessions ("user", device, token, last_usage) values (user_id, create_session.device, create_session.token, CURRENT_DATE);
    else
        update sessions set token = create_session.token, last_usage = CURRENT_DATE where sessions."user" = user_id and sessions.device = create_session.device;
    end if;
end;
$$;


ALTER PROCEDURE flashback.create_session(IN user_id integer, IN token character varying, IN device character varying) OWNER TO flashback;

--
-- Name: create_subject(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_subject(name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare subject integer;
begin
    insert into subjects (name) values (name) returning id into subject;

    return subject;
end;
$$;


ALTER FUNCTION flashback.create_subject(name character varying) OWNER TO flashback;

--
-- Name: create_topic(integer, flashback.expertise_level, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_topic(subject integer, level flashback.expertise_level, name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(topics."position"), 0) + 1 into top_position
    from topics where topics.subject = create_topic.subject and topics.level = create_topic.level;

    insert into topics(subject, level, position, name)
    values (create_topic.subject, create_topic.level, top_position, create_topic.name);

    return top_position;
end;
$$;


ALTER FUNCTION flashback.create_topic(subject integer, level flashback.expertise_level, name character varying) OWNER TO flashback;

--
-- Name: create_topic(integer, flashback.expertise_level, character varying, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_topic(IN subject integer, IN level flashback.expertise_level, IN name character varying, IN "position" integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(topics.position), 0) + 1 into top_position
    from topics where topics.subject = create_topic.subject and topics.level = create_topic.level;

    update topics set position = topics.position + top_position
    where topics.subject = create_topic.subject and topics.level = create_topic.level and topics.position >= create_topic.position;

    insert into topics(subject, level, name, position)
    values (create_topic.subject, create_topic.level, create_topic.name, create_topic.position);

    update topics set position = sub.new_position
    from (
        select row_number() over (order by t.position) as new_position, t.position as old_position, t.subject, t.level
        from topics t where t.subject = create_topic.subject and t.level = create_topic.level
    ) sub
    where sub.subject = topics.subject and sub.level = topics.level and topics.position = sub.old_position;
end;
$$;


ALTER PROCEDURE flashback.create_topic(IN subject integer, IN level flashback.expertise_level, IN name character varying, IN "position" integer) OWNER TO flashback;

--
-- Name: create_user(character varying, character varying, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_user(name character varying, email character varying, hash character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare user_id integer;
begin
    insert into users (name, email, hash) values(name, email, hash) returning id into user_id;

    return user_id;
end;
$$;


ALTER FUNCTION flashback.create_user(name character varying, email character varying, hash character varying) OWNER TO flashback;

--
-- Name: edit_block(integer, integer, text); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_block(IN card integer, IN block integer, IN content text)
    LANGUAGE plpgsql
    AS $$ begin update blocks set content = edit_block.content where blocks.card = edit_block.card and position = block; end; $$;


ALTER PROCEDURE flashback.edit_block(IN card integer, IN block integer, IN content text) OWNER TO flashback;

--
-- Name: edit_card_headline(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_card_headline(IN card integer, IN new_headline character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update cards set headline = new_headline where id = card;
end;
$$;


ALTER PROCEDURE flashback.edit_card_headline(IN card integer, IN new_headline character varying) OWNER TO flashback;

--
-- Name: edit_resource_name(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_name(IN resource integer, IN name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set name = edit_resource_name.name where id = resource;
end;
$$;


ALTER PROCEDURE flashback.edit_resource_name(IN resource integer, IN name character varying) OWNER TO flashback;

--
-- Name: edit_resource_presenter(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_presenter(IN resource integer, IN author character varying)
    LANGUAGE plpgsql
    AS $$ begin update resources set presenter = author where id = resource; end; $$;


ALTER PROCEDURE flashback.edit_resource_presenter(IN resource integer, IN author character varying) OWNER TO flashback;

--
-- Name: edit_resource_provider(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_provider(IN resource integer, IN edited_publisher character varying)
    LANGUAGE plpgsql
    AS $$ begin update resources set provider = edited_publisher where id = resource; end; $$;


ALTER PROCEDURE flashback.edit_resource_provider(IN resource integer, IN edited_publisher character varying) OWNER TO flashback;

--
-- Name: edit_resource_type(integer, flashback.resource_type); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_type(IN resource integer, IN type flashback.resource_type)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set type = edit_resource_type.type where id = resource;
end;
$$;


ALTER PROCEDURE flashback.edit_resource_type(IN resource integer, IN type flashback.resource_type) OWNER TO flashback;

--
-- Name: edit_section_pattern(integer, flashback.section_pattern); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_section_pattern(IN resource integer, IN pattern flashback.section_pattern)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set pattern = edit_section_pattern.pattern
    where resources.id = edit_section_pattern.resource;
end;
$$;


ALTER PROCEDURE flashback.edit_section_pattern(IN resource integer, IN pattern flashback.section_pattern) OWNER TO flashback;

--
-- Name: edit_users_name(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_users_name(IN id integer, IN name character varying)
    LANGUAGE plpgsql
    AS $$ begin update users set name = edit_users_name.name where users.id = edit_users_name.id; end; $$;


ALTER PROCEDURE flashback.edit_users_name(IN id integer, IN name character varying) OWNER TO flashback;

--
-- Name: estimate_read_time(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.estimate_read_time(card_id integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare time_per_word float = 0.4;
declare headline_multiplier integer = 2;
declare text_multiplier integer = 1;
declare code_multiplier integer = 5;
declare headline_weight integer = 0;
declare content_weight integer = 0;
begin
    select array_length(tsvector_to_array(to_tsvector('simple', headline)), 1) * time_per_word * headline_multiplier into headline_weight from cards where id = card_id;
    select sum(array_length(tsvector_to_array(to_tsvector('simple', content)), 1)) * case when type = 'text' then text_multiplier when type = 'code' then code_multiplier end into content_weight from blocks where card = card_id group by content, type;
    return headline_weight + content_weight;
end; $$;


ALTER FUNCTION flashback.estimate_read_time(card_id integer) OWNER TO flashback;

--
-- Name: get_assessment_coverage(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assessment_coverage(assessment_id integer) RETURNS TABLE(subject integer, topic integer, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select a.subject, a.topic, a.level
    from assessments a
    where a.card = assessment_id;
end;
$$;


ALTER FUNCTION flashback.get_assessment_coverage(assessment_id integer) OWNER TO flashback;

--
-- Name: get_assessment_coverage(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assessment_coverage(subject_id integer, topic_position integer, max_level flashback.expertise_level) RETURNS TABLE(card integer, coverage bigint)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select a.card, count(a.topic)
    from assessments a
    where a.card in (
        select aa.card
        from assessments aa
        where aa.subject = subject_id
        and aa.topic = topic_position
        and aa.level <= max_level::expertise_level
    ) group by a.card;
end;
$$;


ALTER FUNCTION flashback.get_assessment_coverage(subject_id integer, topic_position integer, max_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_assessments(integer, integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assessments(user_id integer, subject_id integer, topic_position integer, max_level flashback.expertise_level) RETURNS TABLE(level flashback.expertise_level, assessment integer, assimilations bigint)
    LANGUAGE plpgsql
    AS $$
begin 
    -- 1. find assessment cards covering selected topic: requires get_topic_assessments(user_id, subject_id, topic_position, max_level) returns TABLE(level expertise_level, assessment integer)
    -- 2. find how many topics each of these assessment cards cover: requires get_assessment_coverage(assessment_id) returns TABLE(subject integer, topic integer, level expertise_level)
    -- 3. find which of these topics are assimilated by user: requires get_assimilation_coverage(user_id, assessment_id) returns TABLE(subject integer, topic integer, level expertise_level, assimilated boolean)
    -- 4. find the assessment card that has the widest coverage of topics that user has assimilated: returned by this function

    return query
    select ta.level, ta.assessment, count(*) filter (where ac.assimilated) as assimilations
    from get_topic_assessments(user_id, subject_id, topic_position, 'origin') ta
    cross join lateral get_assimilation_coverage(user_id, ta.assessment) as ac
    group by ta.level, ta.assessment;
end; $$;


ALTER FUNCTION flashback.get_assessments(user_id integer, subject_id integer, topic_position integer, max_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_assimilation_coverage(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assimilation_coverage(user_id integer, assessment_id integer) RETURNS TABLE(subject integer, topic integer, level flashback.expertise_level, assimilated boolean)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select ac.subject, ac.topic, ac.level, bool_and(coalesce(p.progression, 0) >= 3) as assimilated
    from get_assessment_coverage(assessment_id) ac
    join topics_cards tc on tc.subject = ac.subject and tc.topic = ac.topic and tc.level = ac.level
    left join progress p on p.user = user_id and p.card = tc.card
    group by ac.subject, ac.topic, ac.level;
end; $$;


ALTER FUNCTION flashback.get_assimilation_coverage(user_id integer, assessment_id integer) OWNER TO flashback;

--
-- Name: get_blocks(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_blocks(card integer) RETURNS TABLE("position" integer, type flashback.content_type, extension character varying, content text)
    LANGUAGE plpgsql
    AS $$ begin return query select blocks."position", blocks.type, blocks.extension, blocks.content from blocks where blocks.card = get_blocks.card; end; $$;


ALTER FUNCTION flashback.get_blocks(card integer) OWNER TO flashback;

--
-- Name: get_cards(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_cards(user_id integer, subject_id integer, max_level flashback.expertise_level) RETURNS TABLE(topic integer, level flashback.expertise_level, card integer, "position" integer, last_practice timestamp with time zone, duration integer)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select tc.topic, tc.level, tc.card, tc.position, p.last_practice, p.duration
    from topics_cards tc
    left join progress p on p.user = user_id  and p.card = tc.card
    where tc.subject = subject_id and tc.level <= max_level::expertise_level;
end;
$$;


ALTER FUNCTION flashback.get_cards(user_id integer, subject_id integer, max_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_duplicate_card(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_duplicate_card(card_id integer) RETURNS TABLE(rid integer, sid integer, "position" integer, card integer, resource character varying, section character varying, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
declare card_headline text;
begin
    select c.headline into card_headline
    from cards c
    where c.id = card_id;

    return query
    select sc.resource as rid, sc.section as sid, sc.position, sc.card, r.name as resource, se.name as section, c.state, c.headline
    from cards c
    join sections_cards sc on c.id = sc.card
    join sections se on se.resource = sc.resource and se.position = sc.section
    join resources r on r.id = sc.resource
    where c.headline = card_headline and sc.card <> card_id
    order by sc.resource, sc.section, sc.position;
end;
$$;


ALTER FUNCTION flashback.get_duplicate_card(card_id integer) OWNER TO flashback;

--
-- Name: get_lost_cards(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_lost_cards() RETURNS TABLE(sid integer, level flashback.expertise_level, tid integer, "position" integer, card integer, subject character varying, topic character varying, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select tc.subject as sid, tc.level, tc.topic as tid, tc.position, tc.card, s.name as subject, t.name as topic, c.state, c.headline
    from topics_cards tc
    join topics t on t.subject = tc.subject and t.position = tc.topic and t.level = tc.level
    join subjects s on s.id = tc.subject
    join cards c on c.id = tc.card
    where tc.card not in (select sc.card from sections_cards sc)
    order by tc.subject, tc.level, tc.topic, tc.position;
end;
$$;


ALTER FUNCTION flashback.get_lost_cards() OWNER TO flashback;

--
-- Name: get_out_of_shelves(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_out_of_shelves() RETURNS TABLE(id integer, name character varying)
    LANGUAGE plpgsql
    AS $$ begin return query select resources.id, resources.name from resources where resources.id not in (select resource from shelves); end; $$;


ALTER FUNCTION flashback.get_out_of_shelves() OWNER TO flashback;

--
-- Name: get_practice_cards(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_cards(user_id integer, subject_id integer, subject_level flashback.expertise_level) RETURNS TABLE(topic integer, level flashback.expertise_level, card integer, "position" integer, last_practice timestamp with time zone, duration integer)
    LANGUAGE plpgsql
    AS $$
declare last_acceptable_read timestamp with time zone = now() - interval '7 days';
declare long_time_ago timestamp with time zone = now() - interval '100 days';
begin
    if get_practice_mode(user_id, subject_id, subject_level) = 'aggressive'::practice_mode
    then
        return query
        select g.topic, g.level, g.card, g.position, null::timestamp with time zone, null::integer
        from get_cards(user_id, subject_id, subject_level) g
        where coalesce(g.last_practice, long_time_ago) < last_acceptable_read;
    else
        return query
        select g.topic, g.level, g.card, g.position, g.last_practice, g.duration
        from get_cards(user_id, subject_id, subject_level) g;
    end if;
end; $$;


ALTER FUNCTION flashback.get_practice_cards(user_id integer, subject_id integer, subject_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_practice_mode(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_mode(user_id integer, subject_id integer, subject_level flashback.expertise_level) RETURNS flashback.practice_mode
    LANGUAGE plpgsql
    AS $$
declare mode practice_mode;
declare long_time_ago timestamp with time zone = now() - interval '100 days';
declare longest_acceptable_inactivity interval = interval '10 days';
declare longest_acceptable_unreached interval = interval '7 days';
declare most_recent_practice interval;
declare last_recent_practice interval;
declare unread_cards integer;
begin
    select
        now() - coalesce(max(g.last_practice), long_time_ago),
        now() - coalesce(min(g.last_practice), long_time_ago),
        count(*) filter (where g.last_practice is null)
        into most_recent_practice, last_recent_practice, unread_cards
    from get_cards(user_id, subject_id, subject_level) g;

    -- unread cards immediately result in aggressive mode
    -- consequently, users in progressive mode will directly jump into an unread card when a new card drops
    if unread_cards > 0
        -- long interrupts result in memory loss which should be recovered by aggressive mode
        or most_recent_practice >= longest_acceptable_inactivity
        -- any card not being reached by user later than a certain period during progressive practice is considered forgotten and should be reached immediately
        or last_recent_practice >= longest_acceptable_unreached
    then
        select 'aggressive'::practice_mode into mode;
    else
        select 'progressive'::practice_mode into mode;
    end if;

    return mode;
end; $$;


ALTER FUNCTION flashback.get_practice_mode(user_id integer, subject_id integer, subject_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_practice_topics(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_topics(roadmap_id integer) RETURNS TABLE(milestone integer, subject integer, topic integer, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select m.position as milestone, m.subject, t.position as topic, m.level
    from milestones m
    join topics t on t.subject = m.subject and t.level <= m.level
    where m.roadmap = roadmap_id;
end;
$$;


ALTER FUNCTION flashback.get_practice_topics(roadmap_id integer) OWNER TO flashback;

--
-- Name: get_practice_topics(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_topics(roadmap_id integer, subject_id integer) RETURNS TABLE(milestone integer, subject integer, topic integer, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select m.position as milestone, m.subject, t.position as topic, m.level
    from milestones m
    join topics t on t.subject = m.subject and t.level <= m.level
    where m.roadmap = roadmap_id and m.subject = subject_id;
end;
$$;


ALTER FUNCTION flashback.get_practice_topics(roadmap_id integer, subject_id integer) OWNER TO flashback;

--
-- Name: get_resources(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_resources("user" integer) RETURNS TABLE(id integer, name character varying, type flashback.resource_type, condition flashback.condition, presenter character varying, provider character varying, link character varying, last_read timestamp with time zone)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select resources.id, resources.name, resources.type, resources.condition, resources.presenter, resources.provider, resources.link, max(sections_progress.time)
    from resources
    join shelves on shelves.resource = resources.id
    left join sections_progress on sections_progress.resource = resources.id and sections_progress."user" = get_resources."user"
    group by resources.id, resources.name, resources.type, resources.condition, resources.presenter, resources.provider, resources.link;
end;
$$;


ALTER FUNCTION flashback.get_resources("user" integer) OWNER TO flashback;

--
-- Name: get_resources(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_resources(user_id integer, subject_id integer) RETURNS TABLE(id integer, name character varying, type flashback.resource_type, condition flashback.condition, presenter character varying, provider character varying, link character varying, last_read timestamp with time zone)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.id, r.name, r.type, r.condition, r.presenter, r.provider, r.link, max(p.last_practice) filter (where p.last_practice is not null)
    from resources r
    join shelves s on s.resource = r.id and s.subject = subject_id
    join sections_cards sc on sc.resource = r.id
    left join progress p on p."user" = user_id and p.card = sc.card
    group by r.id, r.name, r.type, r.condition, r.presenter, r.provider, r.link;
end;
$$;


ALTER FUNCTION flashback.get_resources(user_id integer, subject_id integer) OWNER TO flashback;

--
-- Name: get_roadmaps(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_roadmaps("user" integer) RETURNS TABLE(id integer, name character varying)
    LANGUAGE plpgsql
    AS $$ begin return query select r.id, r.name from roadmaps r join users_roadmaps ur on ur.roadmap = r.id where ur."user" = get_roadmaps.user order by name; end; $$;


ALTER FUNCTION flashback.get_roadmaps("user" integer) OWNER TO flashback;

--
-- Name: get_sections(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_sections(resource integer) RETURNS TABLE("position" integer, name character varying)
    LANGUAGE plpgsql
    AS $$
declare pattern section_pattern;
begin
    select resources.pattern into pattern from resources where resources.id = get_sections.resource;

    return query select sections.position, coalesce(sections.name, initcap(pattern || ' ' || sections.position)) from sections where sections.resource = get_sections.resource;
end;
$$;


ALTER FUNCTION flashback.get_sections(resource integer) OWNER TO flashback;

--
-- Name: get_sections_cards(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_sections_cards(resource integer, section integer) RETURNS TABLE(id integer, "position" integer, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select cards.id, sc."position", cards.state, cards.headline
    from sections_cards sc
    join cards on cards.id = sc.card
    where sc.resource = get_sections_cards.resource and sc.section = get_sections_cards.section;
end; $$;


ALTER FUNCTION flashback.get_sections_cards(resource integer, section integer) OWNER TO flashback;

--
-- Name: get_subject_resources(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_subject_resources(subject character varying) RETURNS TABLE(id integer, name character varying, type flashback.resource_type, pattern flashback.section_pattern, condition flashback.condition, provider character varying, presenter character varying, link character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.id, r.name, r.type, r.pattern, r.condition, r.provider, r.presenter, r.link
    from resources r
    join shelves v on v.resource = r.id
    join subjects j on j.id = v.subject
    where j.name = get_subject_resources.subject;
end; $$;


ALTER FUNCTION flashback.get_subject_resources(subject character varying) OWNER TO flashback;

--
-- Name: get_subjects(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_subjects(roadmap integer) RETURNS TABLE(level flashback.expertise_level, "position" integer, id integer, name character varying)
    LANGUAGE plpgsql
    AS $$ begin return query select milestones.level, milestones.position, subjects.id, subjects.name from milestones join subjects on milestones.subject = subjects.id where milestones.roadmap = get_subjects.roadmap; end; $$;


ALTER FUNCTION flashback.get_subjects(roadmap integer) OWNER TO flashback;

--
-- Name: get_topic_assessments(integer, integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topic_assessments(user_id integer, subject_id integer, topic_position integer, max_level flashback.expertise_level) RETURNS TABLE(level flashback.expertise_level, assessment integer)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select a.level, a.card
    from assessments a
    where a.subject = subject_id and a.topic = topic_position and a.level <= max_level;
end; $$;


ALTER FUNCTION flashback.get_topic_assessments(user_id integer, subject_id integer, topic_position integer, max_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_topics(integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topics(subject integer, level flashback.expertise_level) RETURNS TABLE("position" integer, name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select topics.position, topics.name
    from topics
    where topics.subject = get_topics.subject and topics.level <= get_topics.level;
end; $$;


ALTER FUNCTION flashback.get_topics(subject integer, level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_topics(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topics(roadmap integer, milestone integer) RETURNS TABLE("position" integer, name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select topics.position, topics.name
    from milestones
    join topics on topics.subject = milestones.subject
    where milestones.roadmap = get_topics.roadmap and milestones.position = get_topics.milestone;
end;
$$;


ALTER FUNCTION flashback.get_topics(roadmap integer, milestone integer) OWNER TO flashback;

--
-- Name: get_topics_cards(integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topics_cards(roadmap_id integer, subject_id integer, topic_position integer) RETURNS TABLE(card integer, "position" integer, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select cards.id, tc."position", cards.state, cards.headline
    from milestones m
    join topics_cards tc on tc.subject = m.subject
    join cards on cards.id = tc.card
    where m.roadmap = roadmap_id
    and m.subject = subject_id
    and tc.topic = topic_position;
end;
$$;


ALTER FUNCTION flashback.get_topics_cards(roadmap_id integer, subject_id integer, topic_position integer) OWNER TO flashback;

--
-- Name: get_unreviewed_sections_cards(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_unreviewed_sections_cards() RETURNS TABLE(rid integer, sid integer, "position" integer, card integer, resource character varying, section character varying, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select sc.resource, sc.section, sc.position, sc.card, r.name, s.name, c.state, c.headline
    from sections_cards sc
    join sections s on s.resource = sc.resource and s.position = sc.section
    join resources r on r.id = sc.resource
    join cards c on c.id = sc.card
    where c.state = 'draft'::card_state
    order by sc.resource, sc.section, sc.position;
end;
$$;


ALTER FUNCTION flashback.get_unreviewed_sections_cards() OWNER TO flashback;

--
-- Name: get_unreviewed_topics_cards(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_unreviewed_topics_cards() RETURNS TABLE(sid integer, level flashback.expertise_level, tid integer, "position" integer, card integer, subject character varying, topic character varying, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select tc.subject as sid, tc.level, tc.topic as tid, tc.position, tc.card, s.name as subject, t.name as topic, c.state, c.headline
    from topics_cards tc
    join topics t on t.subject = tc.subject and t.position = tc.topic and t.level = tc.level
    join subjects s on s.id = tc.subject
    join cards c on c.id = tc.card
    where c.state = 'draft'
    order by tc.subject, tc.level, tc.topic, tc.position;
end;
$$;


ALTER FUNCTION flashback.get_unreviewed_topics_cards() OWNER TO flashback;

--
-- Name: get_unshelved_resources(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_unshelved_resources() RETURNS TABLE(resource integer)
    LANGUAGE plpgsql
    AS $$ begin return query select id from resources where id not in (select s.resource from shelves s); end; $$;


ALTER FUNCTION flashback.get_unshelved_resources() OWNER TO flashback;

--
-- Name: get_user(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_user(user_email character varying) RETURNS TABLE(id integer, name character varying, email character varying, hash character varying, state flashback.user_state, verified boolean, joined timestamp with time zone, token character varying, device character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select u.id, u.name, u.email, u.hash, u.state, u.verified, u.joined, null::character varying, null::character varying
    from users u
    where u.email = user_email;
end; $$;


ALTER FUNCTION flashback.get_user(user_email character varying) OWNER TO flashback;

--
-- Name: get_user(integer, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_user(user_id integer, user_device character varying) RETURNS TABLE(id integer, name character varying, email character varying, hash character varying, state flashback.user_state, verified boolean, joined timestamp with time zone, token character varying, device character varying)
    LANGUAGE plpgsql
    AS $$
begin
    if exists (select id from sessions s where s."user" = user_id and s.device = user_device) then
        update sessions s set last_usage = CURRENT_DATE where s."user" = user_id and s.device = user_device;
    end if;

    return query
    select u.id, u.name, u.email, u.hash, u.state, u.verified, u.joined, s.token, s.device
    from users u
    join sessions s on s."user" = u.id and s."user" = user_id and s.device = user_device;
end; $$;


ALTER FUNCTION flashback.get_user(user_id integer, user_device character varying) OWNER TO flashback;

--
-- Name: is_subject_relevant(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.is_subject_relevant(target_resource integer, target_subject integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$ begin return (select count(cards.id) > 0 from cards join sections_cards s on s.card = cards.id join topics_cards t on t.card = cards.id where s.resource = target_resource and t.subject = target_subject); end; $$;


ALTER FUNCTION flashback.is_subject_relevant(target_resource integer, target_subject integer) OWNER TO flashback;

--
-- Name: log_resources_activities(integer, character varying, integer, flashback.user_action); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.log_resources_activities(IN user_id integer, IN address character varying, IN resource integer, IN action flashback.user_action)
    LANGUAGE plpgsql
    AS $$
begin
    insert into resources_activities ("user", resource, action)
    values (user_id, resource, action);

    insert into network_activities("user", address, activity)
    values (user_id, address, 'upload'::network_activity);
end;
$$;


ALTER PROCEDURE flashback.log_resources_activities(IN user_id integer, IN address character varying, IN resource integer, IN action flashback.user_action) OWNER TO flashback;

--
-- Name: log_sections_activities(integer, character varying, integer, flashback.user_action); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.log_sections_activities(IN user_id integer, IN address character varying, IN section integer, IN action flashback.user_action)
    LANGUAGE plpgsql
    AS $$
begin
    insert into sections_activities ("user", section, action)
    values (user_id, section, action);

    insert into network_activities("user", address, activity)
    values (user_id, address, 'upload'::network_activity);
end;
$$;


ALTER PROCEDURE flashback.log_sections_activities(IN user_id integer, IN address character varying, IN section integer, IN action flashback.user_action) OWNER TO flashback;

--
-- Name: make_progress(integer, integer, integer, flashback.practice_mode); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.make_progress(IN user_id integer, IN card_id integer, IN time_duration integer, IN mode flashback.practice_mode)
    LANGUAGE plpgsql
    AS $$
begin
    insert into progress ("user", card, duration, progression)
    values (user_id, card_id, time_duration, 0)
    on conflict on constraint progress_pkey
    do update set
        duration = time_duration,
        last_practice = now(),
        progression = case when mode = 'progressive'::practice_mode and now() - progress.last_practice > interval '1 hour' then progress.progression + 1 else progress.progression end
    where progress."user" = user_id and progress.card = card_id;
end;
$$;


ALTER PROCEDURE flashback.make_progress(IN user_id integer, IN card_id integer, IN time_duration integer, IN mode flashback.practice_mode) OWNER TO flashback;

--
-- Name: merge_blocks(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_blocks(IN selected_card integer)
    LANGUAGE plpgsql
    AS $$
declare
    top_position integer;
    upper_position integer;
    lower_position integer;
    swap_position integer;
    lower_type content_type;
    lower_extension varchar(20);
begin
    select min(position) into upper_position from blocks where card = selected_card;
    top_position := upper_position;

    -- find the top free position of this practice for swapping
    select coalesce(max(position), 0) + 1 into swap_position from blocks where card = selected_card;

    for lower_position in select position from blocks where card = selected_card and position > top_position order by position loop
        -- collect lower block info
        select type, extension into lower_type, lower_extension
        from blocks where card = selected_card and position = lower_position;

        -- create a new record on the top most position
        insert into blocks (card, position, content, type, extension)
        select selected_card, swap_position, string_agg(coalesce(content, ''), E'\n\n' order by position), lower_type, lower_extension
        from blocks where card = selected_card and position in (upper_position, lower_position);

        -- remove the two merged blocks
        delete from blocks where card = selected_card and position in (upper_position, lower_position);

        -- move the newly created block into the lower position
        update blocks set position = lower_position
        where card = selected_card and position = swap_position;

        upper_position := lower_position;
    end loop;

    -- reorder positions from top to bottom for a practice
    update blocks pb
    set position = sub.new_position
    from (
        select card, position, row_number() over (order by position) as new_position
        from blocks where card = selected_card
    ) sub
    where pb.card = sub.card and pb.position = sub.position;
end;
$$;


ALTER PROCEDURE flashback.merge_blocks(IN selected_card integer) OWNER TO flashback;

--
-- Name: merge_blocks(integer, integer[]); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_blocks(IN selected_card integer, VARIADIC block_list integer[])
    LANGUAGE plpgsql
    AS $$
declare
    upper_position integer;
    lower_position integer;
    swap_position integer;
    block_index integer default 1;
    lower_type content_type;
    lower_extension varchar(20);
begin
    upper_position := block_list[block_index];
    block_index := block_index + 1;

    -- find the top free position of this practice for swapping
    select coalesce(max(position), 0) + 1 into swap_position from blocks where card = selected_card;

    for block_index in 2 .. array_length(block_list, 1) loop
        lower_position := block_list[block_index];

        -- collect lower block info
        select type, extension into lower_type, lower_extension
        from blocks where card = selected_card and position = lower_position;

        -- create a new record on the top most position
        insert into blocks (card, position, content, type, extension)
        select selected_card, swap_position, string_agg(coalesce(content, ''), E'\n\n' order by position), lower_type, lower_extension
        from blocks where card = selected_card and position in (upper_position, lower_position);

        -- remove the two merged blocks
        delete from blocks where card = selected_card and position in (upper_position, lower_position);

        -- move the newly created block into the lower position
        update blocks set position = lower_position
        where card = selected_card and position = swap_position;

        upper_position := lower_position;
    end loop;

    -- reorder positions from top to bottom for a practice
    update blocks pb
    set position = sub.new_position
    from (
        select card, position, row_number() over (order by position) as new_position
        from blocks where card = selected_card
    ) sub
    where pb.card = sub.card and pb.position = sub.position;
end;
$$;


ALTER PROCEDURE flashback.merge_blocks(IN selected_card integer, VARIADIC block_list integer[]) OWNER TO flashback;

--
-- Name: merge_cards(integer, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_cards(IN lhs integer, IN rhs integer, IN new_headline character varying)
    LANGUAGE plpgsql
    AS $$
declare rhs_top_position integer;
begin
    select coalesce(max(position), 0) + 1 into rhs_top_position from blocks where card = lhs;

    update blocks set position = position + rhs_top_position, card = lhs where card = rhs;

    update cards set headline = new_headline where id = lhs;

    if not exists (
        select 1 from topics_cards tc join topics_cards tcc on tcc.subject = tc.subject and tcc.topic = tc.topic and tcc.level = tc.level where tc.card = lhs and tcc.card = rhs
    )
    then
        update topics_cards set card = lhs where card = rhs;
    end if;

    if not exists (
        select 1 from sections_cards sc join sections_cards scc on scc.resource = sc.resource and scc.section = sc.section where sc.card = lhs and scc.card = rhs
    )
    then
        update sections_cards set card = lhs where card = rhs;
    end if;

    delete from cards where id = rhs;
end;
$$;


ALTER PROCEDURE flashback.merge_cards(IN lhs integer, IN rhs integer, IN new_headline character varying) OWNER TO flashback;

--
-- Name: move_card_to_section(integer, integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.move_card_to_section(selected_card integer, current_resource integer, current_section integer, target_section integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare card_position integer;
declare last_position integer;
begin
    select coalesce(position, 1) into card_position from sections_cards where resource = current_resource and section = current_section and card = selected_card;

    if current_section <> target_section then
        select coalesce(max(position), 0) + 1 into last_position from sections_cards where resource = current_resource and section = target_section;
        select last_position into card_position;
        update sections_cards set section = target_section where resource = current_resource and section = current_section and card = selected_card;
        update sections_cards t set position = tt.new_position from (select position, row_number() over (order by position) as new_position from sections_cards where resource = current_resource and section = current_section) tt where t.resource = current_resource and t.section = current_section and t.position = tt.position;
    end if;

    return card_position;
end;
$$;


ALTER FUNCTION flashback.move_card_to_section(selected_card integer, current_resource integer, current_section integer, target_section integer) OWNER TO flashback;

--
-- Name: move_card_to_topic(integer, integer, flashback.expertise_level, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.move_card_to_topic(selected_card integer, current_subject integer, current_level flashback.expertise_level, current_topic integer, target_topic integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare card_position integer;
declare last_position integer;
begin
    select coalesce(position, 1) into card_position from topics_cards where subject = current_subject and topic = target_topic and level = current_level and card = selected_card;

    if current_topic <> target_topic then
        select coalesce(max(position), 0) + 1 into last_position from topics_cards where subject = current_subject and topic = target_topic and level = current_level;
        select last_position into card_position;
        update topics_cards set topic = target_topic, position = last_position where subject = current_subject and topic = current_topic and level = current_level and card = selected_card;
        update topics_cards t set position = tt.new_position from (select position, row_number() over (order by position) as new_position from topics_cards where subject = current_subject and topic = current_topic and level = current_level) tt where t.subject = current_subject and t.topic = current_topic and t.level = current_level and t.position = tt.position;
    end if;

    return card_position;
end;
$$;


ALTER FUNCTION flashback.move_card_to_topic(selected_card integer, current_subject integer, current_level flashback.expertise_level, current_topic integer, target_topic integer) OWNER TO flashback;

--
-- Name: remove_block(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_block(IN card integer, IN block integer)
    LANGUAGE plpgsql
    AS $$ begin delete from blocks where blocks.card = remove_block.card and blocks."position" = remove_block.block; end; $$;


ALTER PROCEDURE flashback.remove_block(IN card integer, IN block integer) OWNER TO flashback;

--
-- Name: rename_roadmap(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_roadmap(IN roadmap integer, IN name character varying)
    LANGUAGE plpgsql
    AS $$ begin update roadmaps set roadmaps.name = rename_roadmap.name where id = roadmap; end; $$;


ALTER PROCEDURE flashback.rename_roadmap(IN roadmap integer, IN name character varying) OWNER TO flashback;

--
-- Name: reorder_blocks(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_blocks(IN target_card integer, IN old_position integer, IN new_position integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    lock table blocks in row exclusive mode;

    select coalesce(max(position), 0) + 1 into top_position from blocks where card = target_card;

    update blocks set position = top_position where card = target_card and position = old_position;

    if new_position < old_position then
        update blocks set position = position + 1 where card = target_card and position >= new_position and position < old_position;
    else
        update blocks set position = position - 1 where card = target_card and position > old_position and position <= new_position;
    end if;

    update blocks set position = new_position where card = target_card and position = top_position;
end;
$$;


ALTER PROCEDURE flashback.reorder_blocks(IN target_card integer, IN old_position integer, IN new_position integer) OWNER TO flashback;

--
-- Name: reorder_milestone(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_milestone(IN roadmap integer, IN old_position integer, IN new_position integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    -- locate the free position on top
    select coalesce(max(m.position), 0) + 1 into top_position from milestones m where m.roadmap = reorder_milestone.roadmap;

    -- relocate targeted milestone to the top
    update milestones set position = top_position where milestones.roadmap = reorder_milestone.roadmap and milestones.position = old_position;

    -- relocate milestones between the old and new positions upwards or downwards to open up a space for targeted milestone on the new location
    if new_position < old_position then
        update milestones set position = milestones.position + 1 where milestones.roadmap = reorder_milestone.roadmap and milestones.position >= new_position and milestones.position < old_position;
    else
        update milestones set position = milestones.position - 1 where milestones.roadmap = reorder_milestone.roadmap and milestones.position > old_position and milestones.position <= new_position;
    end if;

    -- relocate targeted milestone from top to the new free location
    update milestones set position = new_position where milestones.roadmap = reorder_milestone.roadmap and milestones.position = top_position;
end;
$$;


ALTER PROCEDURE flashback.reorder_milestone(IN roadmap integer, IN old_position integer, IN new_position integer) OWNER TO flashback;

--
-- Name: reorder_sections_cards(integer, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_sections_cards(IN target_resource integer, IN target_section integer, IN target_card integer, IN new_position integer)
    LANGUAGE plpgsql
    AS $$
declare old_position integer;
declare top_position integer;
begin
    lock table sections_cards in row exclusive mode;

    select coalesce(max(position), 0) + 1 into top_position
    from sections_cards where section = target_section;

    select position into old_position from sections_cards sc
    where sc.resource = target_resource and sc.section = target_section and sc.card = target_card;

    update sections_cards sc set position = top_position
    where sc.resource = target_resource and sc.section = target_section and sc.card = target_card and sc.position = old_position;

    if new_position < old_position then
        update sections_cards sc set position = sc.position + 1
        where sc.resource = target_resource and sc.section = target_section and sc.position >= new_position and sc.position < old_position;
    else
        update sections_cards sc set position = sc.position - 1
        where sc.resource = target_resource and section = sc.target_section and sc.position > old_position and sc.position <= new_position;
    end if;

    update sections_cards sc set position = new_position
    where sc.resource = target_resource and sc.section = target_section and sc.card = target_card and sc.position = top_position;
end;
$$;


ALTER PROCEDURE flashback.reorder_sections_cards(IN target_resource integer, IN target_section integer, IN target_card integer, IN new_position integer) OWNER TO flashback;

--
-- Name: reorder_topics_cards(integer, flashback.expertise_level, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_topics_cards(IN subject integer, IN level flashback.expertise_level, IN target_topic integer, IN target_card integer, IN new_position integer)
    LANGUAGE plpgsql
    AS $$
declare old_position integer;
declare top_position integer;
begin
    select position into old_position from topics_cards
    where subject = subject and level = level and topic = target_topic and card = target_card;

    select max(position) + 1 into top_position from topics_cards
    where subject = subject and level = level and topic = target_topic;

    update topics_cards set position = top_position
    where subject = subject and level = level and topic = target_topic and card = target_card and position = old_position;

    if new_position < old_position then
        update topics_cards set position = position + 1
        where subject = subject and level = level and topic = target_topic and position >= new_position and position < old_position;
    else
        update topics_cards set position = position - 1
        where subject = subject and level = level and topic = target_topic and position > old_position and position <= new_position;
    end if;

    update topics_cards set position = new_position
    where subject = subject and level = level and topic = target_topic and card = target_card and position = top_position;
end;
$$;


ALTER PROCEDURE flashback.reorder_topics_cards(IN subject integer, IN level flashback.expertise_level, IN target_topic integer, IN target_card integer, IN new_position integer) OWNER TO flashback;

--
-- Name: reset_password(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reset_password(IN user_id integer, IN new_hash character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update users set hash = new_hash where id = user_id;
end; $$;


ALTER PROCEDURE flashback.reset_password(IN user_id integer, IN new_hash character varying) OWNER TO flashback;

--
-- Name: revoke_session(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.revoke_session(IN user_id integer, IN active_token character varying)
    LANGUAGE plpgsql
    AS $$
begin
    delete from sessions where "user" = user_id and token = active_token;
end; $$;


ALTER PROCEDURE flashback.revoke_session(IN user_id integer, IN active_token character varying) OWNER TO flashback;

--
-- Name: revoke_sessions_except(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.revoke_sessions_except(IN user_id integer, IN active_token character varying)
    LANGUAGE plpgsql
    AS $$
begin
    if exists (select device from sessions where "user" = user_id and token = active_token) then
        delete from sessions where "user" = user_id and token <> active_token;
    end if;
end; $$;


ALTER PROCEDURE flashback.revoke_sessions_except(IN user_id integer, IN active_token character varying) OWNER TO flashback;

--
-- Name: split_block(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.split_block(IN card integer, IN block integer)
    LANGUAGE plpgsql
    AS $$
declare parts_count integer;
declare type content_type;
declare extension varchar(20);
declare margin integer;
begin
    create temp table block_parts as
    select row_number() over () - 1 + block as position, part
    from (
        select regexp_split_to_table(content, E'\n\n\n+') as part
        from blocks where blocks.card = split_block.card and blocks.position = block
    );

    select count(position) into parts_count from block_parts;

    select coalesce(max(position), 1) - block into margin
    from blocks where blocks.card = split_block.card;

    select blocks.type, blocks.extension into type, extension
    from blocks where blocks.card = split_block.card and blocks.position = block;

    update blocks set position = position + margin
    where blocks.card = split_block.card and blocks.position > block;

    --update blocks set position = new_position from (
    --    select row_number() over (order by position) - 1 + position + parts_count as new_position, position as old_position
    --    from blocks where blocks.card = split_block.card and blocks.position > block
    --) where blocks.card = split_block.card and blocks.position = old_position;

    delete from blocks where blocks.card = split_block.card and position = block;

    insert into blocks (card, position, content, type, extension)
    select split_block.card, block_parts.position, part, type, extension from block_parts;

    update blocks set position = position - margin + parts_count - 1
    where blocks.card = split_block.card and blocks.position > block + margin;

    drop table block_parts;
end; $$;


ALTER PROCEDURE flashback.split_block(IN card integer, IN block integer) OWNER TO flashback;

--
-- Name: user_exists(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.user_exists(email character varying) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
declare result boolean;
begin
    select exists (select 1 from users where users.email = user_exists.email) into result;
    return result;
end;
$$;


ALTER FUNCTION flashback.user_exists(email character varying) OWNER TO flashback;

--
-- Name: user_is_active(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.user_is_active(email character varying) RETURNS boolean
    LANGUAGE plpgsql
    AS $$ declare result boolean; begin select exists (select 1 from users where users.email = user_is_active.email and users.state = 'active'::user_state) into result; return result; end; $$;


ALTER FUNCTION flashback.user_is_active(email character varying) OWNER TO flashback;

--
-- Name: user_is_verified(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.user_is_verified(email character varying) RETURNS boolean
    LANGUAGE plpgsql
    AS $$ declare result boolean; begin select verified into result from users where users.email = user_is_verified.email; return result; end; $$;


ALTER FUNCTION flashback.user_is_verified(email character varying) OWNER TO flashback;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: assessments; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.assessments (
    topic integer NOT NULL,
    card integer NOT NULL,
    subject integer NOT NULL,
    level flashback.expertise_level NOT NULL
);


ALTER TABLE flashback.assessments OWNER TO flashback;

--
-- Name: authors; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.authors (
    resource integer NOT NULL,
    presenter integer NOT NULL
);


ALTER TABLE flashback.authors OWNER TO flashback;

--
-- Name: blocks; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.blocks (
    card integer NOT NULL,
    "position" integer NOT NULL,
    content text,
    type flashback.content_type NOT NULL,
    extension character varying(20) NOT NULL
);


ALTER TABLE flashback.blocks OWNER TO flashback;

--
-- Name: blocks_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.blocks_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    card integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL,
    "position" integer NOT NULL
);


ALTER TABLE flashback.blocks_activities OWNER TO flashback;

--
-- Name: blocks_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.blocks_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.blocks_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: cards; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.cards (
    id integer NOT NULL,
    headline text,
    state flashback.card_state DEFAULT 'draft'::flashback.card_state NOT NULL
);


ALTER TABLE flashback.cards OWNER TO flashback;

--
-- Name: cards_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.cards_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    card integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL
);


ALTER TABLE flashback.cards_activities OWNER TO flashback;

--
-- Name: cards_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.cards_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.cards_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: cards_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.cards ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.cards_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: milestones; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.milestones (
    subject integer NOT NULL,
    roadmap integer NOT NULL,
    level flashback.expertise_level DEFAULT 'surface'::flashback.expertise_level NOT NULL,
    "position" integer
);


ALTER TABLE flashback.milestones OWNER TO flashback;

--
-- Name: milestones_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.milestones_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL,
    subject integer NOT NULL,
    roadmap integer NOT NULL
);


ALTER TABLE flashback.milestones_activities OWNER TO flashback;

--
-- Name: milestones_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.milestones_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.milestones_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: network_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.network_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL,
    activity flashback.network_activity NOT NULL,
    address character varying(39) NOT NULL
);


ALTER TABLE flashback.network_activities OWNER TO flashback;

--
-- Name: network_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.network_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.network_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: presenters; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.presenters (
    id integer NOT NULL,
    name character varying NOT NULL
);


ALTER TABLE flashback.presenters OWNER TO flashback;

--
-- Name: presenters_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.presenters ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.presenters_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: progress; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.progress (
    "user" integer NOT NULL,
    card integer NOT NULL,
    last_practice timestamp with time zone DEFAULT now() CONSTRAINT progress_time_not_null NOT NULL,
    duration integer NOT NULL,
    progression integer DEFAULT 0 NOT NULL
);


ALTER TABLE flashback.progress OWNER TO flashback;

--
-- Name: resources; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.resources (
    id integer NOT NULL,
    name character varying(200) NOT NULL,
    type flashback.resource_type NOT NULL,
    pattern flashback.section_pattern NOT NULL,
    condition flashback.condition NOT NULL,
    presenter character varying(60),
    provider character varying(60),
    link character varying(2000)
);


ALTER TABLE flashback.resources OWNER TO flashback;

--
-- Name: resources_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.resources_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    resource integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL
);


ALTER TABLE flashback.resources_activities OWNER TO flashback;

--
-- Name: resources_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.resources_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.resources_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: resources_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.resources ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.resources_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: roadmaps; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.roadmaps (
    id integer NOT NULL,
    name character varying(200) NOT NULL
);


ALTER TABLE flashback.roadmaps OWNER TO flashback;

--
-- Name: roadmaps_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.roadmaps_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    roadmap integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL
);


ALTER TABLE flashback.roadmaps_activities OWNER TO flashback;

--
-- Name: roadmaps_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.roadmaps_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.roadmaps_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: roadmaps_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.roadmaps ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.roadmaps_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: sections; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.sections (
    resource integer NOT NULL,
    "position" integer NOT NULL,
    name character varying(200),
    link character varying(2000)
);


ALTER TABLE flashback.sections OWNER TO flashback;

--
-- Name: sections_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.sections_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    resource integer NOT NULL,
    "position" integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL
);


ALTER TABLE flashback.sections_activities OWNER TO flashback;

--
-- Name: sections_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.sections_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.sections_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: sections_cards; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.sections_cards (
    resource integer NOT NULL,
    section integer NOT NULL,
    card integer NOT NULL,
    "position" integer NOT NULL
);


ALTER TABLE flashback.sections_cards OWNER TO flashback;

--
-- Name: sessions; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.sessions (
    "user" integer NOT NULL,
    token character varying(300) NOT NULL,
    device character varying(50),
    last_usage timestamp with time zone
);


ALTER TABLE flashback.sessions OWNER TO flashback;

--
-- Name: shelves; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.shelves (
    resource integer NOT NULL,
    subject integer NOT NULL
);


ALTER TABLE flashback.shelves OWNER TO flashback;

--
-- Name: shelves_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.shelves_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    resource integer NOT NULL,
    subject integer NOT NULL,
    action flashback.user_action,
    "time" timestamp with time zone
);


ALTER TABLE flashback.shelves_activities OWNER TO flashback;

--
-- Name: shelves_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.shelves_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.shelves_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: subjects; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.subjects (
    id integer NOT NULL,
    name character varying(200) NOT NULL
);


ALTER TABLE flashback.subjects OWNER TO flashback;

--
-- Name: subjects_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.subjects_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    subject integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL
);


ALTER TABLE flashback.subjects_activities OWNER TO flashback;

--
-- Name: subjects_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.subjects_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.subjects_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: subjects_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.subjects ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.subjects_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: topics; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.topics (
    "position" integer NOT NULL,
    name character varying(200) NOT NULL,
    subject integer NOT NULL,
    level flashback.expertise_level NOT NULL
);


ALTER TABLE flashback.topics OWNER TO flashback;

--
-- Name: topics_activities; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.topics_activities (
    id integer NOT NULL,
    "user" integer NOT NULL,
    topic integer NOT NULL,
    action flashback.user_action NOT NULL,
    "time" timestamp with time zone DEFAULT now() NOT NULL,
    subject integer NOT NULL,
    level flashback.expertise_level NOT NULL
);


ALTER TABLE flashback.topics_activities OWNER TO flashback;

--
-- Name: topics_activities_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.topics_activities ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.topics_activities_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: topics_cards; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.topics_cards (
    topic integer NOT NULL,
    card integer NOT NULL,
    "position" integer NOT NULL,
    subject integer NOT NULL,
    level flashback.expertise_level NOT NULL
);


ALTER TABLE flashback.topics_cards OWNER TO flashback;

--
-- Name: users; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.users (
    id integer NOT NULL,
    name character varying(30) NOT NULL,
    email character varying(150) NOT NULL,
    state flashback.user_state DEFAULT 'active'::flashback.user_state NOT NULL,
    verified boolean DEFAULT false NOT NULL,
    joined timestamp with time zone DEFAULT now() NOT NULL,
    hash character varying(98)
);


ALTER TABLE flashback.users OWNER TO flashback;

--
-- Name: users_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.users ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.users_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: users_roadmaps; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.users_roadmaps (
    "user" integer NOT NULL,
    roadmap integer NOT NULL
);


ALTER TABLE flashback.users_roadmaps OWNER TO flashback;

--
-- Name: assessments assessments_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.assessments
    ADD CONSTRAINT assessments_pkey PRIMARY KEY (subject, level, topic, card);


--
-- Name: blocks_activities blocks_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.blocks_activities
    ADD CONSTRAINT blocks_activities_pkey PRIMARY KEY (id);


--
-- Name: blocks blocks_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.blocks
    ADD CONSTRAINT blocks_pkey PRIMARY KEY (card, "position");


--
-- Name: cards_activities cards_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.cards_activities
    ADD CONSTRAINT cards_activities_pkey PRIMARY KEY (id);


--
-- Name: cards cards_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.cards
    ADD CONSTRAINT cards_pkey PRIMARY KEY (id);


--
-- Name: milestones_activities milestones_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones_activities
    ADD CONSTRAINT milestones_activities_pkey PRIMARY KEY (id);


--
-- Name: milestones milestones_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones
    ADD CONSTRAINT milestones_pkey PRIMARY KEY (roadmap, subject);


--
-- Name: network_activities network_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.network_activities
    ADD CONSTRAINT network_activities_pkey PRIMARY KEY (id);


--
-- Name: presenters presenters_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.presenters
    ADD CONSTRAINT presenters_pkey PRIMARY KEY (id);


--
-- Name: progress progress_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.progress
    ADD CONSTRAINT progress_pkey PRIMARY KEY ("user", card);


--
-- Name: resources resources_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.resources
    ADD CONSTRAINT resources_pkey PRIMARY KEY (id);


--
-- Name: roadmaps_activities roadmaps_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.roadmaps_activities
    ADD CONSTRAINT roadmaps_activities_pkey PRIMARY KEY (id);


--
-- Name: roadmaps roadmaps_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.roadmaps
    ADD CONSTRAINT roadmaps_pkey PRIMARY KEY (id);


--
-- Name: sections_activities sections_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_activities
    ADD CONSTRAINT sections_activities_pkey PRIMARY KEY (id);


--
-- Name: sections_cards sections_cards_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_cards
    ADD CONSTRAINT sections_cards_pkey PRIMARY KEY (resource, section, card);


--
-- Name: sections sections_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections
    ADD CONSTRAINT sections_pkey PRIMARY KEY (resource, "position");


--
-- Name: sessions sessions_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sessions
    ADD CONSTRAINT sessions_pkey PRIMARY KEY ("user", token);


--
-- Name: sessions sessions_user_device_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sessions
    ADD CONSTRAINT sessions_user_device_key UNIQUE ("user", device);


--
-- Name: shelves_activities shelves_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves_activities
    ADD CONSTRAINT shelves_activities_pkey PRIMARY KEY (id);


--
-- Name: shelves shelves_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves
    ADD CONSTRAINT shelves_pkey PRIMARY KEY (resource, subject);


--
-- Name: subjects_activities subjects_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.subjects_activities
    ADD CONSTRAINT subjects_activities_pkey PRIMARY KEY (id);


--
-- Name: subjects subjects_name_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.subjects
    ADD CONSTRAINT subjects_name_key UNIQUE (name);


--
-- Name: subjects subjects_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.subjects
    ADD CONSTRAINT subjects_pkey PRIMARY KEY (id);


--
-- Name: topics_activities topics_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_activities
    ADD CONSTRAINT topics_activities_pkey PRIMARY KEY (id);


--
-- Name: topics_cards topics_cards_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_cards
    ADD CONSTRAINT topics_cards_pkey PRIMARY KEY (subject, level, topic, card);


--
-- Name: topics topics_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics
    ADD CONSTRAINT topics_pkey PRIMARY KEY (subject, level, "position");


--
-- Name: users_roadmaps user_roadmaps_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.users_roadmaps
    ADD CONSTRAINT user_roadmaps_pkey PRIMARY KEY ("user", roadmap);


--
-- Name: users users_email_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.users
    ADD CONSTRAINT users_email_key UNIQUE (email);


--
-- Name: users users_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.users
    ADD CONSTRAINT users_pkey PRIMARY KEY (id);


--
-- Name: assessments assessments_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.assessments
    ADD CONSTRAINT assessments_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: assessments assessments_subject_level_topic_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.assessments
    ADD CONSTRAINT assessments_subject_level_topic_fkey FOREIGN KEY (subject, level, topic) REFERENCES flashback.topics(subject, level, "position") ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: authors authors_presenter_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.authors
    ADD CONSTRAINT authors_presenter_fkey FOREIGN KEY (presenter) REFERENCES flashback.presenters(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: authors authors_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.authors
    ADD CONSTRAINT authors_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: blocks_activities blocks_activities_card_position_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.blocks_activities
    ADD CONSTRAINT blocks_activities_card_position_fkey FOREIGN KEY (card, "position") REFERENCES flashback.blocks(card, "position") ON UPDATE CASCADE;


--
-- Name: blocks_activities blocks_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.blocks_activities
    ADD CONSTRAINT blocks_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: blocks blocks_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.blocks
    ADD CONSTRAINT blocks_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: cards_activities cards_activities_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.cards_activities
    ADD CONSTRAINT cards_activities_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE;


--
-- Name: cards_activities cards_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.cards_activities
    ADD CONSTRAINT cards_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: milestones_activities milestones_activities_roadmap_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones_activities
    ADD CONSTRAINT milestones_activities_roadmap_subject_fkey FOREIGN KEY (roadmap, subject) REFERENCES flashback.milestones(roadmap, subject) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: milestones_activities milestones_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones_activities
    ADD CONSTRAINT milestones_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: milestones milestones_roadmap_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones
    ADD CONSTRAINT milestones_roadmap_fkey FOREIGN KEY (roadmap) REFERENCES flashback.roadmaps(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: milestones milestones_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones
    ADD CONSTRAINT milestones_subject_fkey FOREIGN KEY (subject) REFERENCES flashback.subjects(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: network_activities network_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.network_activities
    ADD CONSTRAINT network_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: progress progress_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.progress
    ADD CONSTRAINT progress_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: progress progress_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.progress
    ADD CONSTRAINT progress_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: resources_activities resources_activities_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.resources_activities
    ADD CONSTRAINT resources_activities_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: resources_activities resources_activities_resource_fkey1; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.resources_activities
    ADD CONSTRAINT resources_activities_resource_fkey1 FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE;


--
-- Name: resources_activities resources_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.resources_activities
    ADD CONSTRAINT resources_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: roadmaps_activities roadmaps_activities_roadmap_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.roadmaps_activities
    ADD CONSTRAINT roadmaps_activities_roadmap_fkey FOREIGN KEY (roadmap) REFERENCES flashback.roadmaps(id) ON UPDATE CASCADE;


--
-- Name: roadmaps_activities roadmaps_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.roadmaps_activities
    ADD CONSTRAINT roadmaps_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: sections_activities sections_activities_resource_position_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_activities
    ADD CONSTRAINT sections_activities_resource_position_fkey FOREIGN KEY (resource, "position") REFERENCES flashback.sections(resource, "position") ON UPDATE CASCADE;


--
-- Name: sections_activities sections_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_activities
    ADD CONSTRAINT sections_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: sections_cards sections_cards_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_cards
    ADD CONSTRAINT sections_cards_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: sections_cards sections_cards_resource_position_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_cards
    ADD CONSTRAINT sections_cards_resource_position_fkey FOREIGN KEY (resource, section) REFERENCES flashback.sections(resource, "position") ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: sections sections_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections
    ADD CONSTRAINT sections_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: sessions sessions_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sessions
    ADD CONSTRAINT sessions_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: shelves_activities shelves_activities_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves_activities
    ADD CONSTRAINT shelves_activities_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: shelves_activities shelves_activities_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves_activities
    ADD CONSTRAINT shelves_activities_subject_fkey FOREIGN KEY (subject) REFERENCES flashback.subjects(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: shelves_activities shelves_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves_activities
    ADD CONSTRAINT shelves_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: shelves shelves_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves
    ADD CONSTRAINT shelves_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: shelves shelves_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.shelves
    ADD CONSTRAINT shelves_subject_fkey FOREIGN KEY (subject) REFERENCES flashback.subjects(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: subjects_activities subjects_activities_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.subjects_activities
    ADD CONSTRAINT subjects_activities_subject_fkey FOREIGN KEY (subject) REFERENCES flashback.subjects(id) ON UPDATE CASCADE;


--
-- Name: subjects_activities subjects_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.subjects_activities
    ADD CONSTRAINT subjects_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: topics_activities topics_activities_subject_level_position_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_activities
    ADD CONSTRAINT topics_activities_subject_level_position_fkey FOREIGN KEY (subject, level, topic) REFERENCES flashback.topics(subject, level, "position") ON UPDATE CASCADE;


--
-- Name: topics_activities topics_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_activities
    ADD CONSTRAINT topics_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: topics_cards topics_cards_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_cards
    ADD CONSTRAINT topics_cards_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: topics_cards topics_cards_subject_level_topic_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_cards
    ADD CONSTRAINT topics_cards_subject_level_topic_fkey FOREIGN KEY (subject, level, topic) REFERENCES flashback.topics(subject, level, "position") ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: users_roadmaps user_roadmaps_roadmap_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.users_roadmaps
    ADD CONSTRAINT user_roadmaps_roadmap_fkey FOREIGN KEY (roadmap) REFERENCES flashback.roadmaps(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: users_roadmaps user_roadmaps_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.users_roadmaps
    ADD CONSTRAINT user_roadmaps_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

\unrestrict dGsHx9ipa2gODb2S9cKl6h2eqF1cIWxNxqwFxazD2Qp4zcIwKBbbxjjQBrh6Alf

