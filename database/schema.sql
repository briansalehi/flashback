--
-- PostgreSQL database dump
--

\restrict 8M8TEeWEskUJ4ARsIP8wFWASXdZbdYUNMs9fzOUzo27AggF7iehSikPrUcGKpuh

-- Dumped from database version 18.1
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
-- Name: public; Type: SCHEMA; Schema: -; Owner: flashback
--

-- *not* creating schema, since initdb creates it


ALTER SCHEMA public OWNER TO flashback;

--
-- Name: citext; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS citext WITH SCHEMA flashback;


--
-- Name: EXTENSION citext; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION citext IS 'data type for case-insensitive character strings';


--
-- Name: pg_trgm; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pg_trgm WITH SCHEMA flashback;


--
-- Name: EXTENSION pg_trgm; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION pg_trgm IS 'text similarity measurement and index searching based on trigrams';


--
-- Name: card_state; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.card_state AS ENUM (
    'draft',
    'reviewed',
    'completed',
    'approved',
    'released',
    'rejected'
);


ALTER TYPE flashback.card_state OWNER TO flashback;

--
-- Name: closure_state; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.closure_state AS ENUM (
    'draft',
    'reviewed',
    'completed'
);


ALTER TYPE flashback.closure_state OWNER TO flashback;

--
-- Name: content_type; Type: TYPE; Schema: flashback; Owner: flashback
--

CREATE TYPE flashback.content_type AS ENUM (
    'text',
    'code',
    'image'
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
    'nerve'
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
    'playlist',
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
-- Name: add_card_to_section(integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.add_card_to_section(card_id integer, resource_id integer, section_position integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(position), 0) + 1 into top_position from section_cards sc where sc.resource = resource_id and sc.section = section_position;

    insert into section_cards (resource, section, card, position) values (resource_id, section_position, card_id, top_position);

    return top_position;
end;
$$;


ALTER FUNCTION flashback.add_card_to_section(card_id integer, resource_id integer, section_position integer) OWNER TO flashback;

--
-- Name: add_card_to_topic(integer, integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.add_card_to_topic(card_id integer, subject_id integer, topic_position integer, topic_level flashback.expertise_level) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(t.position), 0) + 1 into top_position from topic_cards t where t.subject = subject_id and t.level = topic_level and t.topic = topic_position;

    insert into topic_cards (subject, level, topic, card, position) values (subject_id, topic_level, topic_position, card_id, top_position);

    return top_position;
end;
$$;


ALTER FUNCTION flashback.add_card_to_topic(card_id integer, subject_id integer, topic_position integer, topic_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: add_milestone(integer, flashback.expertise_level, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.add_milestone(subject_id integer, subject_level flashback.expertise_level, roadmap_id integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(milestones.position), 0) + 1 into top_position from milestones where milestones.roadmap = roadmap_id;

    insert into milestones(roadmap, subject, level, position) values (roadmap_id, subject_id, subject_level, top_position);

    return top_position;
end;
$$;


ALTER FUNCTION flashback.add_milestone(subject_id integer, subject_level flashback.expertise_level, roadmap_id integer) OWNER TO flashback;

--
-- Name: add_milestone(integer, flashback.expertise_level, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_milestone(IN subject_id integer, IN subject_level flashback.expertise_level, IN roadmap_id integer, IN subject_position integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(milestones.position), 0) + 1 into top_position from milestones where milestones.roadmap = roadmap_id;

    update milestones m set position = m.position + top_position where m.roadmap = roadmap_id and m.position >= subject_position;

    insert into milestones(roadmap, subject, level, position) values (
        roadmap_id, subject_id, subject_level, subject_position
    );

    update milestones m set position = s.altered_position
    from (
        select mm.position, mm.level, row_number() over (order by mm.position) as altered_position from milestones mm where mm.roadmap = roadmap_id
    ) s
    where m.roadmap = roadmap_id and m.level = s.level and m.position = s.position;
end;
$$;


ALTER PROCEDURE flashback.add_milestone(IN subject_id integer, IN subject_level flashback.expertise_level, IN roadmap_id integer, IN subject_position integer) OWNER TO flashback;

--
-- Name: add_presenter(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_presenter(IN resource_id integer, IN presenter_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into authors (resource, presenter) values (resource_id, presenter_id);
end;
$$;


ALTER PROCEDURE flashback.add_presenter(IN resource_id integer, IN presenter_id integer) OWNER TO flashback;

--
-- Name: add_provider(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_provider(IN resource_id integer, IN provider_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into producers (resource, provider) values (resource_id, provider_id);
end;
$$;


ALTER PROCEDURE flashback.add_provider(IN resource_id integer, IN provider_id integer) OWNER TO flashback;

--
-- Name: add_requirement(integer, integer, flashback.expertise_level, integer, flashback.expertise_level); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_requirement(IN roadmap_id integer, IN subject_id integer, IN subject_level flashback.expertise_level, IN required_subject_id integer, IN minimum_subject_level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    if
        -- a subject cannot be its own requirement
        subject_id <> required_subject_id and
        -- a subject cannot have circular requirement
        not exists (select 1 from requirements where roadmap = roadmap_id and subject = required_subject_id and required_subject = subject_id) and
        -- a requirement weaker than what already exists for a subject is a dupliate and is avoided
        not exists (select 1 from requirements where roadmap = roadmap_id and subject = subject_id and level < subject_level and required_subject = required_subject_id and minimum_level >= minimum_subject_level)
    then
        insert into requirements (roadmap, subject, level, required_subject, minimum_level)
        values (roadmap_id, subject_id, subject_level, required_subject_id, minimum_subject_level);
    end if;
end; $$;


ALTER PROCEDURE flashback.add_requirement(IN roadmap_id integer, IN subject_id integer, IN subject_level flashback.expertise_level, IN required_subject_id integer, IN minimum_subject_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: add_resource_to_subject(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.add_resource_to_subject(IN resource_id integer, IN subject_id integer)
    LANGUAGE plpgsql
    AS $$ begin insert into shelves (resource, subject) values (resource_id, subject_id); end; $$;


ALTER PROCEDURE flashback.add_resource_to_subject(IN resource_id integer, IN subject_id integer) OWNER TO flashback;

--
-- Name: change_block_type(integer, integer, flashback.content_type); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_block_type(IN card_id integer, IN block_position integer, IN block_type flashback.content_type)
    LANGUAGE plpgsql
    AS $$
begin
    update blocks set type = block_type where card = card_id and position = block_position;
end;
$$;


ALTER PROCEDURE flashback.change_block_type(IN card_id integer, IN block_position integer, IN block_type flashback.content_type) OWNER TO flashback;

--
-- Name: change_milestone_level(integer, integer, flashback.expertise_level); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_milestone_level(IN roadmap_id integer, IN subject_id integer, IN subject_level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    update milestones set level = subject_level where roadmap = roadmap_id and subject = subject_id;
end; $$;


ALTER PROCEDURE flashback.change_milestone_level(IN roadmap_id integer, IN subject_id integer, IN subject_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: change_resource_type(integer, flashback.resource_type); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_resource_type(IN resource_id integer, IN resource_type flashback.resource_type)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set type = resource_type where id = resource_id;
end;
$$;


ALTER PROCEDURE flashback.change_resource_type(IN resource_id integer, IN resource_type flashback.resource_type) OWNER TO flashback;

--
-- Name: change_section_pattern(integer, flashback.section_pattern); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_section_pattern(IN resource_id integer, IN resource_pattern flashback.section_pattern)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set pattern = resource_pattern where resources.id = resource_id;
end;
$$;


ALTER PROCEDURE flashback.change_section_pattern(IN resource_id integer, IN resource_pattern flashback.section_pattern) OWNER TO flashback;

--
-- Name: change_topic_level(integer, integer, flashback.expertise_level, flashback.expertise_level); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_topic_level(IN subject_id integer, IN topic_position integer, IN topic_level flashback.expertise_level, IN target_level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    if topic_level <> target_level then
        update topics set level = target_level where subject = subject_id and level = topic_level and position = topic_position;
    end if;
end; $$;


ALTER PROCEDURE flashback.change_topic_level(IN subject_id integer, IN topic_position integer, IN topic_level flashback.expertise_level, IN target_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: change_users_hash(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.change_users_hash(IN user_id integer, IN hash character varying)
    LANGUAGE plpgsql
    AS $$ begin update users set hash = change_users_hash.hash where users.id = change_users_hash.user_id; end; $$;


ALTER PROCEDURE flashback.change_users_hash(IN user_id integer, IN hash character varying) OWNER TO flashback;

--
-- Name: clone_roadmap(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.clone_roadmap(user_id integer, roadmap_id integer) RETURNS TABLE(id integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
declare roadmap_name citext;
declare roadmap_owner integer;
declare cloned_roadmap_id integer;
begin
    select r.name, r.user into roadmap_name, roadmap_owner from roadmaps r where r.id = roadmap_id;

    if roadmap_owner <> user_id then
        cloned_roadmap_id := create_roadmap(user_id, roadmap_name);
        insert into milestones (roadmap, subject, level, position) select m.roadmap, m.subject, m.level, m.position from milestones m where m.roadmap = roadmap_id;
    end if;
end; $$;


ALTER FUNCTION flashback.clone_roadmap(user_id integer, roadmap_id integer) OWNER TO flashback;

--
-- Name: create_assessment(integer, flashback.expertise_level, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_assessment(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN card_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into assessments (subject, level, topic, card) values (subject_id, topic_level, topic_position, card_id);
end;
$$;


ALTER PROCEDURE flashback.create_assessment(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN card_id integer) OWNER TO flashback;

--
-- Name: create_block(integer, flashback.content_type, character varying, text, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_block(card_id integer, block_type flashback.content_type, block_extension character varying, block_content text, block_metadata character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare next_position integer;
begin
    select coalesce(max(position), 0) + 1 into next_position from blocks b where b.card = card_id;

    insert into blocks (card, type, extension, content, metadata, position) values (card_id, block_type, block_extension, block_content, block_metadata, next_position);

    return next_position;
end;
$$;


ALTER FUNCTION flashback.create_block(card_id integer, block_type flashback.content_type, block_extension character varying, block_content text, block_metadata character varying) OWNER TO flashback;

--
-- Name: create_block(integer, flashback.content_type, character varying, text, character varying, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_block(IN card_id integer, IN block_type flashback.content_type, IN block_extension character varying, IN block_content text, IN block_metadata character varying, IN block_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into blocks (card, type, extension, content, metadata, position) values (card_id, block_type, block_extension, block_content, block_metadata, block_position);
end;
$$;


ALTER PROCEDURE flashback.create_block(IN card_id integer, IN block_type flashback.content_type, IN block_extension character varying, IN block_content text, IN block_metadata character varying, IN block_position integer) OWNER TO flashback;

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
-- Name: create_nerve(integer, character varying, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_nerve(user_id integer, resource_name character varying, subject_id integer, expiration integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare user_name character varying;
declare resource_id integer;
begin
    select name into user_name from users where id = user_id;

    resource_id := create_resource(resource_name, 'nerve'::resource_type, 'synapse'::section_pattern, null::character varying, cast(extract(epoch from now()) as integer), expiration);

    insert into shelves (resource, subject) values (resource_id, subject_id);

    insert into nerves ("user", resource, subject) values (user_id, resource_id, subject_id);

    return resource_id;
end; $$;


ALTER FUNCTION flashback.create_nerve(user_id integer, resource_name character varying, subject_id integer, expiration integer) OWNER TO flashback;

--
-- Name: create_presenter(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_presenter(presenter_name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare presenter_id integer;
begin
    insert into presenters (name) values (presenter_name) returning id into presenter_id;

    return presenter_id;
end;
$$;


ALTER FUNCTION flashback.create_presenter(presenter_name character varying) OWNER TO flashback;

--
-- Name: create_provider(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_provider(provider_name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare provider_id integer;
begin
    insert into providers (name) values (provider_name) returning id into provider_id;

    return provider_id;
end;
$$;


ALTER FUNCTION flashback.create_provider(provider_name character varying) OWNER TO flashback;

--
-- Name: create_resource(character varying, flashback.resource_type, flashback.section_pattern, character varying, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_resource(resource_name character varying, resource_type flashback.resource_type, resource_pattern flashback.section_pattern, resource_link character varying, resource_production integer, resource_expiration integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare resource_id integer;
begin
    insert into resources (name, type, pattern, link, production, expiration)
    values (resource_name, resource_type, resource_pattern, nullif(resource_link, ''), resource_production, resource_expiration)
    returning id into resource_id;

    return resource_id;
end;
$$;


ALTER FUNCTION flashback.create_resource(resource_name character varying, resource_type flashback.resource_type, resource_pattern flashback.section_pattern, resource_link character varying, resource_production integer, resource_expiration integer) OWNER TO flashback;

--
-- Name: create_roadmap(integer, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_roadmap(user_id integer, roadmap_name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare roadmap_id integer;
begin
    insert into roadmaps ("user", name) values (user_id, roadmap_name) returning id into roadmap_id;

    return roadmap_id;
end;
$$;


ALTER FUNCTION flashback.create_roadmap(user_id integer, roadmap_name character varying) OWNER TO flashback;

--
-- Name: create_section(integer, character varying, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.create_section(resource_id integer, resource_name character varying, resource_link character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare last_position integer;
begin
    select coalesce(max(position), 0) + 1 into last_position from sections where sections.resource = resource_id;

    insert into sections (resource, position, name, link) values (resource_id, last_position, nullif(resource_name, ''), nullif(resource_link, ''));

    return last_position;
end;
$$;


ALTER FUNCTION flashback.create_section(resource_id integer, resource_name character varying, resource_link character varying) OWNER TO flashback;

--
-- Name: create_section(integer, character varying, character varying, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_section(IN resource_id integer, IN section_name character varying, IN section_link character varying, IN section_position integer)
    LANGUAGE plpgsql
    AS $$
declare last_position integer;
begin
    if not exists (select 1 from resources where resources.id = resource_id) then
        raise notice 'Resource does not exist';
    end if;

    select max(coalesce(s.position, 0)) into last_position from sections s where s.resource = resource_id;

    update sections s set position = s.position + last_position where s.resource = resource_id and s.position >= section_position;

    insert into sections (resource, position, name) values (resource_id, section_position, nullif(section_name, ''));

    update sections s set position = ss.new_position
    from (
        select row_number() over (order by s.position) as new_position, s.position from sections s where s.resource = resource_id
    ) ss
    where s.resource = resource_id and ss.position = s.position;
end;
$$;


ALTER PROCEDURE flashback.create_section(IN resource_id integer, IN section_name character varying, IN section_link character varying, IN section_position integer) OWNER TO flashback;

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

CREATE FUNCTION flashback.create_topic(subject_id integer, topic_level flashback.expertise_level, topic_name character varying) RETURNS integer
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(topics."position"), 0) + 1 into top_position
    from topics where topics.subject = subject_id and topics.level = topic_level;

    insert into topics(subject, level, position, name)
    values (subject_id, topic_level, top_position, topic_name);

    return top_position;
end;
$$;


ALTER FUNCTION flashback.create_topic(subject_id integer, topic_level flashback.expertise_level, topic_name character varying) OWNER TO flashback;

--
-- Name: create_topic(integer, flashback.expertise_level, character varying, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.create_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_name character varying, IN topic_position integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select coalesce(max(topics.position), 0) + 1 into top_position from topics where topics.subject = subject_id and topics.level = topic_level;

    update topics set position = topics.position + top_position where topics.subject = subject_id and topics.level = topic_level and topics.position >= topic_position;

    insert into topics(subject, level, name, position) values (subject_id, topic_level, topic_name, topic_position);

    update topics set position = sub.new_position
    from (
        select row_number() over (order by t.position) as new_position, t.position as old_position, t.subject, t.level
        from topics t where t.subject = subject_id and t.level = topic_level
    ) sub
    where sub.subject = topics.subject and sub.level = topics.level and topics.position = sub.old_position;
end;
$$;


ALTER PROCEDURE flashback.create_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_name character varying, IN topic_position integer) OWNER TO flashback;

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
-- Name: diminish_assessment(integer, integer, flashback.expertise_level, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.diminish_assessment(IN card_id integer, IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from assessments where subject = subject_id and level = topic_level and topic = topic_position and card = card_id;
end;
$$;


ALTER PROCEDURE flashback.diminish_assessment(IN card_id integer, IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer) OWNER TO flashback;

--
-- Name: drop_presenter(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.drop_presenter(IN resource_id integer, IN presenter_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from authors where resource = resource_id and presenter = presenter_id;
end;
$$;


ALTER PROCEDURE flashback.drop_presenter(IN resource_id integer, IN presenter_id integer) OWNER TO flashback;

--
-- Name: drop_provider(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.drop_provider(IN resource_id integer, IN provider_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from producers where resource = resource_id and provider = provider_id;
end;
$$;


ALTER PROCEDURE flashback.drop_provider(IN resource_id integer, IN provider_id integer) OWNER TO flashback;

--
-- Name: drop_resource_from_subject(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.drop_resource_from_subject(IN resource_id integer, IN subject_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from shelves where resource = resource_id and subject = subject_id;
end;
$$;


ALTER PROCEDURE flashback.drop_resource_from_subject(IN resource_id integer, IN subject_id integer) OWNER TO flashback;

--
-- Name: edit_block(integer, integer, text); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_block(IN card integer, IN block integer, IN content text)
    LANGUAGE plpgsql
    AS $$ begin update blocks set content = edit_block.content where blocks.card = edit_block.card and position = block; end; $$;


ALTER PROCEDURE flashback.edit_block(IN card integer, IN block integer, IN content text) OWNER TO flashback;

--
-- Name: edit_block_content(integer, integer, text); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_block_content(IN card_id integer, IN block_position integer, IN block_content text)
    LANGUAGE plpgsql
    AS $$
begin
    update blocks set content = block_content where card = card_id and position = block_position;
end; $$;


ALTER PROCEDURE flashback.edit_block_content(IN card_id integer, IN block_position integer, IN block_content text) OWNER TO flashback;

--
-- Name: edit_block_extension(integer, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_block_extension(IN card_id integer, IN block_position integer, IN block_extension character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update blocks set extension = block_extension where card = card_id and position = block_position;
end;
$$;


ALTER PROCEDURE flashback.edit_block_extension(IN card_id integer, IN block_position integer, IN block_extension character varying) OWNER TO flashback;

--
-- Name: edit_block_metadata(integer, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_block_metadata(IN card_id integer, IN block_position integer, IN block_metadata character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update blocks set metadata = block_metadata where card = card_id and position = block_position;
end;
$$;


ALTER PROCEDURE flashback.edit_block_metadata(IN card_id integer, IN block_position integer, IN block_metadata character varying) OWNER TO flashback;

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
-- Name: edit_resource_expiration(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_expiration(IN resource_id integer, IN resource_expiration integer)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set expiration = resource_expiration where id = resource_id;
end;
$$;


ALTER PROCEDURE flashback.edit_resource_expiration(IN resource_id integer, IN resource_expiration integer) OWNER TO flashback;

--
-- Name: edit_resource_link(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_link(IN resource_id integer, IN subject_link character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set link = subject_link where id = resource_id;
end;
$$;


ALTER PROCEDURE flashback.edit_resource_link(IN resource_id integer, IN subject_link character varying) OWNER TO flashback;

--
-- Name: edit_resource_presenter(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_presenter(IN resource integer, IN author character varying)
    LANGUAGE plpgsql
    AS $$ begin update resources set presenter = author where id = resource; end; $$;


ALTER PROCEDURE flashback.edit_resource_presenter(IN resource integer, IN author character varying) OWNER TO flashback;

--
-- Name: edit_resource_production(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_production(IN resource_id integer, IN resource_production integer)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set production = resource_production where id = resource_id;
end;
$$;


ALTER PROCEDURE flashback.edit_resource_production(IN resource_id integer, IN resource_production integer) OWNER TO flashback;

--
-- Name: edit_resource_provider(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_resource_provider(IN resource integer, IN edited_publisher character varying)
    LANGUAGE plpgsql
    AS $$ begin update resources set provider = edited_publisher where id = resource; end; $$;


ALTER PROCEDURE flashback.edit_resource_provider(IN resource integer, IN edited_publisher character varying) OWNER TO flashback;

--
-- Name: edit_section_link(integer, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.edit_section_link(IN resource_id integer, IN section_position integer, IN section_link character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update sections set link = section_link where resource = resource_id and position = section_position;
end; $$;


ALTER PROCEDURE flashback.edit_section_link(IN resource_id integer, IN section_position integer, IN section_link character varying) OWNER TO flashback;

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
-- Name: expand_assessment(integer, integer, flashback.expertise_level, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.expand_assessment(IN card_id integer, IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into assessments (subject, level, topic, card) values (subject_id, topic_level, topic_position, card_id);
end;
$$;


ALTER PROCEDURE flashback.expand_assessment(IN card_id integer, IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer) OWNER TO flashback;

--
-- Name: get_assessment_coverage(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assessment_coverage(subject_id integer, topic_position integer, max_level flashback.expertise_level) RETURNS TABLE(id integer, state flashback.card_state, headline flashback.citext, coverage bigint)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select c.id, c.state, c.headline, count(a.topic)
    from assessments a
    join cards c on c.id = a.card
    where a.card in (
        select aa.card
        from assessments aa
        where aa.subject = subject_id
        and aa.topic = topic_position
        and aa.level <= max_level::expertise_level
    ) group by c.id, c.state, c.headline;
end;
$$;


ALTER FUNCTION flashback.get_assessment_coverage(subject_id integer, topic_position integer, max_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_assessments(integer, integer, flashback.expertise_level, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assessments(user_id integer, subject_id integer, topic_level flashback.expertise_level, topic_position integer) RETURNS TABLE(id integer, state flashback.card_state, headline flashback.citext, assimilations bigint)
    LANGUAGE plpgsql
    AS $$
begin 
    -- 1. find assessment cards covering selected topic: requires get_topic_assessments(user_id, subject_id, topic_position, max_level) returns TABLE(level expertise_level, assessment integer)
    -- 2. find how many topics each of these assessment cards cover: requires get_assessment_coverage(assessment_id) returns TABLE(subject integer, topic integer, level expertise_level)
    -- 3. find which of these topics are assimilated by user: requires get_assimilation_coverage(user_id, assessment_id) returns TABLE(subject integer, topic integer, level expertise_level, assimilated boolean)
    -- 4. find the assessment card that has the widest coverage of topics that user has assimilated: returned by this function

    return query
    select ta.id, ta.state, ta.headline, count(*) filter (where ac.assimilated) as assimilations
    from get_topic_assessments(user_id, subject_id, topic_position, topic_level) ta
    cross join lateral get_assimilation_coverage(user_id, subject_id, ta.id) as ac
    group by ta.id, ta.state, ta.headline;
end; $$;


ALTER FUNCTION flashback.get_assessments(user_id integer, subject_id integer, topic_level flashback.expertise_level, topic_position integer) OWNER TO flashback;

--
-- Name: get_assimilation_coverage(integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_assimilation_coverage(user_id integer, subject_id integer, assessment_id integer) RETURNS TABLE("position" integer, level flashback.expertise_level, name flashback.citext, assimilated boolean)
    LANGUAGE plpgsql
    AS $$
declare longest_acceptable_inactivity timestamp = now() - '10 days'::interval;
begin
    return query
    select i.position, i.level, i.name, bool_and(coalesce(p.progression, 0) >= 3) as assimilated
    from get_topic_coverage(subject_id, assessment_id) c
    join topic_cards t on t.subject = subject_id and c.level = t.level and c.position = t.topic
    join topics i on i.subject = t.subject and i.level = t.level and i.position = t.topic
    left join progress p on p."user" = user_id and p.card = t.card and p.last_practice > longest_acceptable_inactivity
    group by t.subject, i.position, i.level, i.name;
end; $$;


ALTER FUNCTION flashback.get_assimilation_coverage(user_id integer, subject_id integer, assessment_id integer) OWNER TO flashback;

--
-- Name: get_blocks(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_blocks(card_id integer) RETURNS TABLE("position" integer, type flashback.content_type, extension character varying, content text, metadata character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query select b.position, b.type, b.extension, b.content, b.metadata from blocks b where b.card = card_id;
end;
$$;


ALTER FUNCTION flashback.get_blocks(card_id integer) OWNER TO flashback;

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
    join section_cards sc on c.id = sc.card
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
    from topic_cards tc
    join topics t on t.subject = tc.subject and t.position = tc.topic and t.level = tc.level
    join subjects s on s.id = tc.subject
    join cards c on c.id = tc.card
    where tc.card not in (select sc.card from section_cards sc)
    order by tc.subject, tc.level, tc.topic, tc.position;
end;
$$;


ALTER FUNCTION flashback.get_lost_cards() OWNER TO flashback;

--
-- Name: get_milestones(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_milestones(roadmap_id integer) RETURNS TABLE(level flashback.expertise_level, "position" integer, id integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select milestones.level, milestones.position, subjects.id, subjects.name
    from milestones
    join subjects on milestones.subject = subjects.id
    where milestones.roadmap = roadmap_id;
end; $$;


ALTER FUNCTION flashback.get_milestones(roadmap_id integer) OWNER TO flashback;

--
-- Name: get_nerves(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_nerves(user_id integer) RETURNS TABLE(id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, link character varying, production integer, expiration integer)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    from resources r
    join nerves n on n.resource = r.id
    where n."user" = user_id;
end;
$$;


ALTER FUNCTION flashback.get_nerves(user_id integer) OWNER TO flashback;

--
-- Name: get_out_of_shelves(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_out_of_shelves() RETURNS TABLE(id integer, name character varying)
    LANGUAGE plpgsql
    AS $$ begin return query select resources.id, resources.name from resources where provider <> 'Flashback'::character varying and resources.id not in (select resource from shelves); end; $$;


ALTER FUNCTION flashback.get_out_of_shelves() OWNER TO flashback;

--
-- Name: get_practice_cards(integer, integer, integer, flashback.expertise_level, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_cards(user_id integer, roadmap_id integer, subject_id integer, topic_level flashback.expertise_level, topic_position integer) RETURNS TABLE(id integer, state flashback.card_state, headline flashback.citext)
    LANGUAGE plpgsql
    AS $$
declare cognitive_level expertise_level;
declare last_acceptable_read timestamp with time zone = now() - interval '7 days';
declare long_time_ago timestamp with time zone = now() - interval '100 days';
begin
    cognitive_level := get_user_cognitive_level(user_id, roadmap_id, subject_id);

    return query
    select c.id, c.state, c.headline
    from topic_cards tc
    join cards c on c.id = tc.card
    left join progress p on p.user = user_id and p.card = tc.card
    where tc.subject = subject_id and tc.level <= cognitive_level and tc.level = topic_level and tc.topic = topic_position
    and (
        get_practice_mode(user_id, subject_id, cognitive_level) <> 'aggressive'::practice_mode
        or coalesce(p.last_practice, long_time_ago) < last_acceptable_read
    );
end; $$;


ALTER FUNCTION flashback.get_practice_cards(user_id integer, roadmap_id integer, subject_id integer, topic_level flashback.expertise_level, topic_position integer) OWNER TO flashback;

--
-- Name: get_practice_mode(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_mode(user_id integer, subject_id integer, topic_level flashback.expertise_level) RETURNS flashback.practice_mode
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
        now() - coalesce(max(p.last_practice), long_time_ago),
        now() - coalesce(min(p.last_practice), long_time_ago),
        count(*) filter (where p.last_practice is null)
        into most_recent_practice, last_recent_practice, unread_cards
    from topic_cards tc
    left join progress p on p.user = user_id and p.card = tc.card
    where tc.subject = subject_id and tc.level <= topic_level;

    -- unread cards immediately result in aggressive mode
    -- consequently, users in progressive mode will temporarily switch to aggressive when a new card is available
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


ALTER FUNCTION flashback.get_practice_mode(user_id integer, subject_id integer, topic_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_practice_topics(integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_practice_topics(user_id integer, roadmap_id integer, subject_id integer) RETURNS TABLE("position" integer, name flashback.citext, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select t.position, t.name, t.level
    from topics t
    where t.subject = subject_id and t.level <= get_user_cognitive_level(user_id, roadmap_id, subject_id);
end;
$$;


ALTER FUNCTION flashback.get_practice_topics(user_id integer, roadmap_id integer, subject_id integer) OWNER TO flashback;

--
-- Name: get_progress_weight(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_progress_weight(user_id integer) RETURNS TABLE(id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, production integer, expiration integer, link character varying, percentage bigint)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.id, r.name, r.type, r.pattern, r.production, r.expiration, r.link, count(p.card) * 100 / count(c.card)
    from progress p right
    join section_cards c on c.card = p.card
    join resources r on r.id = c.resource
    where p."user" = user_id and p.last_practice >= CURRENT_DATE::timestamp
    group by r.id, r.name, r.type, r.pattern, r.production, r.expiration, r.link;
end; $$;


ALTER FUNCTION flashback.get_progress_weight(user_id integer) OWNER TO flashback;

--
-- Name: get_requirements(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_requirements(roadmap_id integer, subject_id integer, subject_level flashback.expertise_level) RETURNS TABLE(subject integer, "position" integer, name flashback.citext, required_level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.required_subject, m.position, s.name, max(r.minimum_level)
    from requirements r
    join milestones m on m.roadmap = r.roadmap and m.subject = r.required_subject and r.minimum_level <= m.level
    join subjects s on s.id = r.required_subject
    where r.roadmap = roadmap_id and r.subject = subject_id and r.level <= subject_level
    group by r.required_subject, m.position, s.name;
end; $$;


ALTER FUNCTION flashback.get_requirements(roadmap_id integer, subject_id integer, subject_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_resource_state(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_resource_state(resource_id integer) RETURNS flashback.closure_state
    LANGUAGE plpgsql
    AS $$
declare resource_state closure_state;
begin
    select min(state) into resource_state from sections where resource = resource_id group by state;
    return coalesce(resource_state, 'draft'::closure_state);
end; $$;


ALTER FUNCTION flashback.get_resource_state(resource_id integer) OWNER TO flashback;

--
-- Name: get_resources(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_resources(user_id integer, subject_id integer) RETURNS TABLE(id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, production integer, expiration integer, link character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.id, r.name, r.type, r.pattern, r.production, r.expiration, r.link
    from resources r
    join shelves s on s.resource = r.id and s.subject = subject_id
    where (r.type <> 'nerve'::resource_type or r.id in (select n.resource from nerves n where n."user" = user_id));
end;
$$;


ALTER FUNCTION flashback.get_resources(user_id integer, subject_id integer) OWNER TO flashback;

--
-- Name: get_roadmaps(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_roadmaps("user" integer) RETURNS TABLE(id integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    return query select r.id, r.name from roadmaps r where r."user" = get_roadmaps.user;
end; $$;


ALTER FUNCTION flashback.get_roadmaps("user" integer) OWNER TO flashback;

--
-- Name: get_section_cards(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_section_cards(resource_id integer, section_position integer) RETURNS TABLE(id integer, state flashback.card_state, headline flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    return query select c.id, c.state, c.headline from section_cards sc join cards c on c.id = sc.card where sc.resource = resource_id and sc.section = section_position;
end;
$$;


ALTER FUNCTION flashback.get_section_cards(resource_id integer, section_position integer) OWNER TO flashback;

--
-- Name: get_sections(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_sections(resource_id integer) RETURNS TABLE("position" integer, state flashback.closure_state, name flashback.citext, link character varying)
    LANGUAGE plpgsql
    AS $$
declare pattern section_pattern;
begin
    select r.pattern into pattern from resources r where r.id = resource_id;

    return query select s.position, s.state, coalesce(s.name, initcap(pattern || ' ' || s.position)::citext), s.link from sections s where s.resource = resource_id;
end;
$$;


ALTER FUNCTION flashback.get_sections(resource_id integer) OWNER TO flashback;

--
-- Name: get_study_resources(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_study_resources(user_id integer) RETURNS TABLE("position" bigint, id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, link character varying, production integer, expiration integer)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select row_number() over (order by max(i.last_study) desc), r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    from roadmaps a
    join milestones m on m.roadmap = a.id
    join shelves h on h.subject = m.subject
    join resources r on r.id = h.resource and case r.type when 'nerve'::resource_type then r.id in (select n.resource from nerves n where n."user" = user_id) else true end
    join sections s on s.resource = r.id and s.state < 'completed'
    join section_cards c on c.resource = r.id and c.section = s.position
    join studies i on i.card = c.card
    where a."user" = user_id
    group by r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration;
end; $$;


ALTER FUNCTION flashback.get_study_resources(user_id integer) OWNER TO flashback;

--
-- Name: get_study_resources_variation_call(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_study_resources_variation_call(user_id integer) RETURNS TABLE("position" bigint, id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, link character varying, production integer, expiration integer)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select row_number() over (order by max(i.last_study)), r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    from roadmaps a
    join milestones m on m.roadmap = a.id
    join shelves h on h.subject = m.subject
    join resources r on r.id = h.resource and get_resource_state(r.id) < 'completed'
    join section_cards c on c.resource = r.id
    join studies i on i.card = c.card
    where a."user" = user_id
    group by r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    order by r.id;
end; $$;


ALTER FUNCTION flashback.get_study_resources_variation_call(user_id integer) OWNER TO flashback;

--
-- Name: get_study_resources_variation_join(integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_study_resources_variation_join(user_id integer) RETURNS TABLE("position" bigint, id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, link character varying, production integer, expiration integer)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select row_number() over (order by max(i.last_study)), r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    from roadmaps a
    join milestones m on m.roadmap = a.id
    join shelves h on h.subject = m.subject
    join resources r on r.id = h.resource
    join sections s on s.resource = r.id and s.state < 'completed'
    join section_cards c on c.resource = r.id and c.section = s.position
    join studies i on i.card = c.card
    where a."user" = user_id
    group by r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    order by r.id;
end; $$;


ALTER FUNCTION flashback.get_study_resources_variation_join(user_id integer) OWNER TO flashback;

--
-- Name: get_subject_resources(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_subject_resources(subject character varying) RETURNS TABLE(id integer, name character varying, type flashback.resource_type, pattern flashback.section_pattern, expiration date, provider character varying, presenter character varying, link character varying)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select r.id, r.name, r.type, r.pattern, r.date, r.provider, r.presenter, r.link
    from resources r
    join shelves v on v.resource = r.id
    join subjects j on j.id = v.subject
    where j.name = get_subject_resources.subject;
end; $$;


ALTER FUNCTION flashback.get_subject_resources(subject character varying) OWNER TO flashback;

--
-- Name: get_topic_assessments(integer, integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topic_assessments(user_id integer, subject_id integer, topic_position integer, max_level flashback.expertise_level) RETURNS TABLE(id integer, state flashback.card_state, headline flashback.citext, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select c.id, c.state, c.headline, a.level
    from assessments a
    join cards c on c.id = a.card
    where a.subject = subject_id and a.topic = topic_position and a.level <= max_level;
end; $$;


ALTER FUNCTION flashback.get_topic_assessments(user_id integer, subject_id integer, topic_position integer, max_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_topic_cards(integer, integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topic_cards(subject_id integer, topic_position integer, topic_level flashback.expertise_level) RETURNS TABLE(id integer, state flashback.card_state, headline flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    return query select c.id, c.state, c.headline from topic_cards tc join cards c on c.id = tc.card where tc.subject = subject_id and tc.topic = topic_position and tc.level = topic_level;
end;
$$;


ALTER FUNCTION flashback.get_topic_cards(subject_id integer, topic_position integer, topic_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_topic_coverage(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topic_coverage(subject_id integer, assessment_id integer) RETURNS TABLE("position" integer, level flashback.expertise_level, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select t.position, t.level, t.name
    from assessments a
    join topics t on t.subject = a.subject and t.level = a.level and t.position = a.topic
    where a.subject = subject_id and a.card = assessment_id;
end;
$$;


ALTER FUNCTION flashback.get_topic_coverage(subject_id integer, assessment_id integer) OWNER TO flashback;

--
-- Name: get_topics(integer, flashback.expertise_level); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_topics(subject_id integer, topic_level flashback.expertise_level) RETURNS TABLE("position" integer, name flashback.citext, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    return query select t.position, t.name, t.level from topics t where t.subject = subject_id and t.level = topic_level;
end; $$;


ALTER FUNCTION flashback.get_topics(subject_id integer, topic_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: get_unreviewed_section_cards(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_unreviewed_section_cards() RETURNS TABLE(rid integer, sid integer, "position" integer, card integer, resource character varying, section character varying, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select sc.resource, sc.section, sc.position, sc.card, r.name, s.name, c.state, c.headline
    from section_cards sc
    join sections s on s.resource = sc.resource and s.position = sc.section
    join resources r on r.id = sc.resource
    join cards c on c.id = sc.card
    where c.state = 'draft'::card_state
    order by sc.resource, sc.section, sc.position;
end;
$$;


ALTER FUNCTION flashback.get_unreviewed_section_cards() OWNER TO flashback;

--
-- Name: get_unreviewed_topic_cards(); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_unreviewed_topic_cards() RETURNS TABLE(sid integer, level flashback.expertise_level, tid integer, "position" integer, card integer, subject character varying, topic character varying, state flashback.card_state, headline text)
    LANGUAGE plpgsql
    AS $$
begin
    return query
    select tc.subject as sid, tc.level, tc.topic as tid, tc.position, tc.card, s.name as subject, t.name as topic, c.state, c.headline
    from topic_cards tc
    join topics t on t.subject = tc.subject and t.position = tc.topic and t.level = tc.level
    join subjects s on s.id = tc.subject
    join cards c on c.id = tc.card
    where c.state = 'draft'
    order by tc.subject, tc.level, tc.topic, tc.position;
end;
$$;


ALTER FUNCTION flashback.get_unreviewed_topic_cards() OWNER TO flashback;

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
-- Name: get_user(character varying, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_user(user_token character varying, user_device character varying) RETURNS TABLE(id integer, name character varying, email character varying, hash character varying, state flashback.user_state, verified boolean, joined timestamp with time zone, token character varying, device character varying)
    LANGUAGE plpgsql
    AS $$
begin
    if exists (select id from sessions s where s.token = user_token and s.device = user_device) then
        update sessions s set last_usage = CURRENT_DATE where s.token = user_token and s.device = user_device;
    end if;

    return query
    select u.id, u.name, u.email, u.hash, u.state, u.verified, u.joined, s.token, s.device
    from users u
    join sessions s on s."user" = u.id and s.token = user_token and s.device = user_device;
end; $$;


ALTER FUNCTION flashback.get_user(user_token character varying, user_device character varying) OWNER TO flashback;

--
-- Name: get_user_cognitive_level(integer, integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.get_user_cognitive_level(user_id integer, roadmap_id integer, subject_id integer) RETURNS flashback.expertise_level
    LANGUAGE plpgsql
    AS $$
declare cognitive_level expertise_level;
declare mode practice_mode;
begin
    select t.level into cognitive_level
    from topics t
    join milestones m on m.subject = t.subject
    where t.subject = subject_id and t.level <= m.level and get_practice_mode(user_id, t.subject, t.level) = 'aggressive'
    group by t.subject, t.level
    order by t.level
    limit 1;

    if cognitive_level is null then
        select t.level into cognitive_level
        from topics t
        join milestones m on m.subject = t.subject
        where t.subject = subject_id and t.level <= m.level and get_practice_mode(user_id, t.subject, t.level) = 'progressive'
        group by t.subject, t.level
        order by t.level desc
        limit 1;
    end if;

    return cognitive_level;
end; $$;


ALTER FUNCTION flashback.get_user_cognitive_level(user_id integer, roadmap_id integer, subject_id integer) OWNER TO flashback;

--
-- Name: is_assimilated(integer, integer, flashback.expertise_level, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.is_assimilated(user_id integer, subject_id integer, topic_level flashback.expertise_level, topic_position integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
declare longest_acceptable_inactivity timestamp = now() - '10 days'::interval;
declare assimilated bool;
begin
    select bool_and(coalesce(p.progression, 0) >= 3) into assimilated
    from topic_cards t
    left join progress p on p."user" = user_id and p.card = t.card and p.last_practice > longest_acceptable_inactivity
    where t.subject = subject_id and t.level = topic_level and t.topic = topic_position
    group by t.subject, t.level, t.topic;

    return assimilated;
end; $$;


ALTER FUNCTION flashback.is_assimilated(user_id integer, subject_id integer, topic_level flashback.expertise_level, topic_position integer) OWNER TO flashback;

--
-- Name: is_subject_relevant(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.is_subject_relevant(target_resource integer, target_subject integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
begin
    return (
        select count(cards.id) > 0
        from cards
        join section_cards s on s.card = cards.id
        join topic_cards t on t.card = cards.id
        where s.resource = target_resource and t.subject = target_subject
    );
end; $$;


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
-- Name: mark_card_as_approved(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.mark_card_as_approved(IN card_id integer)
    LANGUAGE plpgsql
    AS $$
declare current_state card_state;
begin
    select state into current_state from cards where id = card_id;

    if current_state = 'completed'::card_state then
        update cards set state = 'approved'::card_state where id = card_id;
    end if;
end; $$;


ALTER PROCEDURE flashback.mark_card_as_approved(IN card_id integer) OWNER TO flashback;

--
-- Name: mark_card_as_completed(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.mark_card_as_completed(IN card_id integer)
    LANGUAGE plpgsql
    AS $$
declare current_state card_state;
begin
    select state into current_state from cards where id = card_id;

    if current_state = 'reviewed'::card_state then
        update cards set state = 'completed'::card_state where id = card_id;
    end if;
end; $$;


ALTER PROCEDURE flashback.mark_card_as_completed(IN card_id integer) OWNER TO flashback;

--
-- Name: mark_card_as_released(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.mark_card_as_released(IN card_id integer)
    LANGUAGE plpgsql
    AS $$
declare current_state card_state;
begin
    select state into current_state from cards where id = card_id;

    if current_state = 'approved'::card_state then
        update cards set state = 'released'::card_state where id = card_id;
    end if;
end; $$;


ALTER PROCEDURE flashback.mark_card_as_released(IN card_id integer) OWNER TO flashback;

--
-- Name: mark_card_as_reviewed(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.mark_card_as_reviewed(IN card_id integer)
    LANGUAGE plpgsql
    AS $$
declare current_state card_state;
begin
    select state into current_state from cards where id = card_id;

    if current_state = 'draft'::card_state or current_state = 'completed'::card_state then
        update cards set state = 'reviewed'::card_state where id = card_id;
    end if;
end; $$;


ALTER PROCEDURE flashback.mark_card_as_reviewed(IN card_id integer) OWNER TO flashback;

--
-- Name: mark_section_as_completed(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.mark_section_as_completed(IN resource_id integer, IN section_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    update sections set state = 'completed'::closure_state where resource = resource_id and position = section_position;
end; $$;


ALTER PROCEDURE flashback.mark_section_as_completed(IN resource_id integer, IN section_position integer) OWNER TO flashback;

--
-- Name: mark_section_as_reviewed(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.mark_section_as_reviewed(IN resource_id integer, IN section_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    update sections set state = 'reviewed'::closure_state where resource = resource_id and position = section_position;
end; $$;


ALTER PROCEDURE flashback.mark_section_as_reviewed(IN resource_id integer, IN section_position integer) OWNER TO flashback;

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

CREATE PROCEDURE flashback.merge_cards(IN source_id integer, IN target_id integer, IN target_headline character varying)
    LANGUAGE plpgsql
    AS $$
declare target_top_block_position integer;
begin
    select coalesce(max(position), 0) + 1 into target_top_block_position from blocks where card = target_id;

    update blocks set position = position + target_top_block_position, card = target_id where card = source_id;

    update cards set headline = target_headline where id = target_id;

    if not exists (select 1 from topic_cards tc join topic_cards tcc on tcc.subject = tc.subject and tcc.topic = tc.topic and tcc.level = tc.level where tc.card = source_id and tcc.card = target_id)
    then
        update topic_cards set card = target_id where card = source_id;
    end if;

    if not exists (
        select 1 from section_cards sc join section_cards scc on scc.resource = sc.resource and scc.section = sc.section where sc.card = source_id and scc.card = target_id
    )
    then
        update section_cards set card = target_id where card = source_id;
    end if;

    delete from cards where id = source_id;
end;
$$;


ALTER PROCEDURE flashback.merge_cards(IN source_id integer, IN target_id integer, IN target_headline character varying) OWNER TO flashback;

--
-- Name: merge_presenters(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_presenters(IN source_id integer, IN target_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    update authors set presenter = target_id where presenter = source_id and resource not in (select resource from authors where presenter = target_id);

    delete from presenters where id = source_id;
end;
$$;


ALTER PROCEDURE flashback.merge_presenters(IN source_id integer, IN target_id integer) OWNER TO flashback;

--
-- Name: merge_providers(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_providers(IN source_id integer, IN target_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    update producers set provider = target_id where provider = source_id and resource not in (select resource from producers where provider = target_id);

    delete from providers where id = source_id;
end;
$$;


ALTER PROCEDURE flashback.merge_providers(IN source_id integer, IN target_id integer) OWNER TO flashback;

--
-- Name: merge_resources(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_resources(IN source_id integer, IN target_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    if source_id <> target_id then
        update sections set resource = target_id where resource = source_id;

        update shelves set resource = target_id where resource = source_id and subject not in (select subject from shelves where resource = target_id);

        update nerves set resource = target_id where resource = source_id;

        update authors set resource = target_id where resource = source_id;

        update producers set resource = target_id where resource = source_id;

        delete from resources where id = source_id;
    end if;
end;
$$;


ALTER PROCEDURE flashback.merge_resources(IN source_id integer, IN target_id integer) OWNER TO flashback;

--
-- Name: merge_sections(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_sections(IN resource_id integer, IN source_section integer, IN target_section integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select max(coalesce(position, 0)) into top_position from section_cards where resource = resource_id and section = target_section;

    update section_cards set section = target_section, position = position + top_position where resource = resource_id and section = source_section;

    delete from sections where resource = resource_id and position = source_section;

    update sections s set position = ss.updated_position from (
        select si.position, row_number() over (order by si.position) as updated_position, si.resource from sections si where resource = resource_id
    ) as ss
    where s.resource = ss.resource and s.position = ss.position;

end;
$$;


ALTER PROCEDURE flashback.merge_sections(IN resource_id integer, IN source_section integer, IN target_section integer) OWNER TO flashback;

--
-- Name: merge_subjects(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_subjects(IN source_subject_id integer, IN target_subject_id integer)
    LANGUAGE plpgsql
    AS $$
declare last_position integer;
begin
    if source_subject_id <> target_subject_id then
        select max(coalesce(position, 0)) + 1 into last_position from topics where subject = source_subject_id;
        update topics set position = t.position + last_position, subject = target_subject_id where subject = source_subject_id;
    end if;
end; $$;


ALTER PROCEDURE flashback.merge_subjects(IN source_subject_id integer, IN target_subject_id integer) OWNER TO flashback;

--
-- Name: merge_topics(integer, flashback.expertise_level, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.merge_topics(IN subject_id integer, IN topic_level flashback.expertise_level, IN source_topic integer, IN target_topic integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    select max(coalesce(position, 0)) into top_position from topic_cards where subject = subject_id and level = topic_level and topic = target_topic;

    update topic_cards set topic = target_topic, position = position + top_position where subject = subject_id and level = topic_level and topic = source_topic;

    delete from topics where subject = subject_id and level = topic_level and position = source_topic;

    update topics t set position = tt.updated_position from (
        select ti.position, row_number() over (order by ti.position) as updated_position, ti.subject, ti.level from topics ti where ti.subject = subject_id and ti.level = topic_level
    ) as tt
    where t.subject = tt.subject and t.level = tt.level and t.position = tt.position;

end;
$$;


ALTER PROCEDURE flashback.merge_topics(IN subject_id integer, IN topic_level flashback.expertise_level, IN source_topic integer, IN target_topic integer) OWNER TO flashback;

--
-- Name: move_block(integer, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.move_block(IN card_id integer, IN block_position integer, IN target_card integer, IN target_position integer)
    LANGUAGE plpgsql
    AS $$
declare source_top_position integer;
declare target_top_position integer;
begin
    select max(coalesce(position, 0)) + 1 into source_top_position from blocks where card = card_id;

    select max(coalesce(position, 0)) + 1 into target_top_position from blocks where card = target_card;

    if target_position > 0 and block_position > 0 then
        call reorder_block(card_id, block_position, source_top_position);

        update blocks set card = target_card, position = target_top_position where card = card_id and position = source_top_position;

        call reorder_block(target_card, target_top_position, target_position);
    end if;
end; $$;


ALTER PROCEDURE flashback.move_block(IN card_id integer, IN block_position integer, IN target_card integer, IN target_position integer) OWNER TO flashback;

--
-- Name: move_card_to_section(integer, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.move_card_to_section(IN card_id integer, IN resource_id integer, IN current_section integer, IN target_section integer)
    LANGUAGE plpgsql
    AS $$
declare card_position integer;
declare last_position integer;
begin
    select coalesce(position, 1) into card_position from section_cards where resource = resource_id and section = current_section and card = card_id;

    if current_section <> target_section then
        select coalesce(max(position), 0) + 1 into last_position from section_cards where resource = resource_id and section = target_section;

        update section_cards set section = target_section, position = last_position where resource = resource_id and section = current_section and card = card_id;

        update section_cards t set position = tt.updated_position from (
            select position, row_number() over (order by position) as updated_position from section_cards where resource = resource_id and section = current_section
        ) tt where t.resource = resource_id and t.section = current_section and t.position = tt.position;
    end if;
end;
$$;


ALTER PROCEDURE flashback.move_card_to_section(IN card_id integer, IN resource_id integer, IN current_section integer, IN target_section integer) OWNER TO flashback;

--
-- Name: move_card_to_topic(integer, integer, integer, flashback.expertise_level, integer, integer, flashback.expertise_level); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.move_card_to_topic(IN card_id integer, IN subject_id integer, IN topic_position integer, IN topic_level flashback.expertise_level, IN target_subject integer, IN target_topic integer, IN target_level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
declare last_position integer;
begin
    if (subject_id <> target_subject) or (subject_id = target_subject and topic_position <> target_topic)
    then
        select coalesce(max(position), 0) + 1 into last_position from topic_cards where subject = target_subject and topic = target_topic and level = target_level;

        update topic_cards set subject = target_subject, topic = target_topic, position = last_position, level = target_level where subject = subject_id and topic = topic_position and level = topic_level and card = card_id;

        update topic_cards t set position = tt.updated_position from (
            select position, row_number() over (order by position) as updated_position from topic_cards where subject = subject_id and topic = topic_position and level = topic_level
        ) tt where t.subject = subject_id and t.topic = topic_position and t.level = topic_level and t.position = tt.position;
    end if;
end;
$$;


ALTER PROCEDURE flashback.move_card_to_topic(IN card_id integer, IN subject_id integer, IN topic_position integer, IN topic_level flashback.expertise_level, IN target_subject integer, IN target_topic integer, IN target_level flashback.expertise_level) OWNER TO flashback;

--
-- Name: move_section(integer, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.move_section(IN resource_id integer, IN section_position integer, IN target_resource_id integer, IN target_section_position integer)
    LANGUAGE plpgsql
    AS $$
declare source_top_position integer;
declare target_top_position integer;
begin
    select max(coalesce(position, 0)) + 1 into source_top_position from sections where resource = resource_id;

    select max(coalesce(position, 0)) + 1 into target_top_position from sections where resource = target_resource_id;

    if target_section_position > 0 and section_position > 0 then
        call reorder_section(resource_id, section_position, source_top_position);

        update sections set resource = target_resource_id, position = target_top_position where resource = resource_id and position = source_top_position;

        call reorder_section(target_resource_id, target_top_position, target_section_position);
    end if;
end; $$;


ALTER PROCEDURE flashback.move_section(IN resource_id integer, IN section_position integer, IN target_resource_id integer, IN target_section_position integer) OWNER TO flashback;

--
-- Name: move_topic(integer, flashback.expertise_level, integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.move_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN target_subject_id integer, IN target_topic_position integer)
    LANGUAGE plpgsql
    AS $$
declare source_top_position integer;
declare target_top_position integer;
begin
    select max(coalesce(position, 0)) + 1 into source_top_position from topics where subject = subject_id and level = topic_level;

    select max(coalesce(position, 0)) + 1 into target_top_position from topics where subject = target_subject_id and level = topic_level;

    if target_topic_position > 0 and topic_position > 0 then
        call reorder_topic(subject_id, topic_level, topic_position, source_top_position);

        update topics set subject = target_subject_id, position = target_top_position where subject = subject_id and level = topic_level and position = source_top_position;

        call reorder_topic(target_subject_id, topic_level, target_top_position, target_topic_position);
    end if;
end; $$;


ALTER PROCEDURE flashback.move_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN target_subject_id integer, IN target_topic_position integer) OWNER TO flashback;

--
-- Name: remove_block(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_block(IN card_id integer, IN block_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from blocks where card = card_id and "position" = block_position;

    update blocks set position = position - 1 where card = card_id and "position" > block_position;
end; $$;


ALTER PROCEDURE flashback.remove_block(IN card_id integer, IN block_position integer) OWNER TO flashback;

--
-- Name: remove_card(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_card(IN card_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from cards where id = card_id;
end;
$$;


ALTER PROCEDURE flashback.remove_card(IN card_id integer) OWNER TO flashback;

--
-- Name: remove_milestone(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_milestone(IN roadmap_id integer, IN subject_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from milestones where roadmap = roadmap_id and subject = subject_id;
end; $$;


ALTER PROCEDURE flashback.remove_milestone(IN roadmap_id integer, IN subject_id integer) OWNER TO flashback;

--
-- Name: remove_presenter(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_presenter(IN presenter_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from presenters where id = presenter_id;
end;
$$;


ALTER PROCEDURE flashback.remove_presenter(IN presenter_id integer) OWNER TO flashback;

--
-- Name: remove_provider(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_provider(IN provider_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from providers where id = provider_id;
end;
$$;


ALTER PROCEDURE flashback.remove_provider(IN provider_id integer) OWNER TO flashback;

--
-- Name: remove_resource(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_resource(IN resource_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    if not exists (select 1 from section_cards where resource = resource_id) then
        delete from resources where id = resource_id;
    end if;
end;
$$;


ALTER PROCEDURE flashback.remove_resource(IN resource_id integer) OWNER TO flashback;

--
-- Name: remove_roadmap(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_roadmap(IN roadmap_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    delete from roadmaps where id = roadmap_id;
end; $$;


ALTER PROCEDURE flashback.remove_roadmap(IN roadmap_id integer) OWNER TO flashback;

--
-- Name: remove_section(integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_section(IN resource_id integer, IN section_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    if not exists (select 1 from section_cards where resource = resource_id) then
        delete from sections where resource = resource_id and position = section_position;
    end if;
end;
$$;


ALTER PROCEDURE flashback.remove_section(IN resource_id integer, IN section_position integer) OWNER TO flashback;

--
-- Name: remove_subject(integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_subject(IN subject_id integer)
    LANGUAGE plpgsql
    AS $$
begin
    if not exists (select 1 from topic_cards where subject = subject_id) then
        delete from subjects where id = subject_id;
    end if;
end; $$;


ALTER PROCEDURE flashback.remove_subject(IN subject_id integer) OWNER TO flashback;

--
-- Name: remove_topic(integer, flashback.expertise_level, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.remove_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer)
    LANGUAGE plpgsql
    AS $$
begin
    if not exists (select 1 from topic_cards where subject = subject_id) then
        delete from topics where subject = subject_id and level = topic_level and position = topic_position;

        update topics set position = position - 1 where subject = subject_id and level = topic_level and position > topic_position;
    end if;
end;
$$;


ALTER PROCEDURE flashback.remove_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer) OWNER TO flashback;

--
-- Name: rename_presenter(integer, flashback.citext); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_presenter(IN presenter_id integer, IN presenter_name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    update presenters set name = presenter_name where id = presenter_id;
end;
$$;


ALTER PROCEDURE flashback.rename_presenter(IN presenter_id integer, IN presenter_name flashback.citext) OWNER TO flashback;

--
-- Name: rename_provider(integer, flashback.citext); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_provider(IN provider_id integer, IN provider_name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    update providers set name = provider_name where id = provider_id;
end;
$$;


ALTER PROCEDURE flashback.rename_provider(IN provider_id integer, IN provider_name flashback.citext) OWNER TO flashback;

--
-- Name: rename_resource(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_resource(IN resource_id integer, IN resource_name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update resources set name = resource_name where id = resource_id;
end;
$$;


ALTER PROCEDURE flashback.rename_resource(IN resource_id integer, IN resource_name character varying) OWNER TO flashback;

--
-- Name: rename_roadmap(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_roadmap(IN roadmap_id integer, IN roadmap_name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update roadmaps set name = roadmap_name where id = roadmap_id;
end; $$;


ALTER PROCEDURE flashback.rename_roadmap(IN roadmap_id integer, IN roadmap_name character varying) OWNER TO flashback;

--
-- Name: rename_section(integer, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_section(IN resource_id integer, IN section_position integer, IN section_name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update sections set name = section_name where resource = resource_id and position = section_position;
end; $$;


ALTER PROCEDURE flashback.rename_section(IN resource_id integer, IN section_position integer, IN section_name character varying) OWNER TO flashback;

--
-- Name: rename_subject(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_subject(IN subject_id integer, IN subject_name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update subjects set name = subject_name where id = subject_id;
end
$$;


ALTER PROCEDURE flashback.rename_subject(IN subject_id integer, IN subject_name character varying) OWNER TO flashback;

--
-- Name: rename_topic(integer, flashback.expertise_level, integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN topic_name character varying)
    LANGUAGE plpgsql
    AS $$
begin
    update topics set name = topic_name where subject = subject_id and level = topic_level and position = topic_position;
end; $$;


ALTER PROCEDURE flashback.rename_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN topic_name character varying) OWNER TO flashback;

--
-- Name: rename_user(integer, character varying); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.rename_user(IN id integer, IN name character varying)
    LANGUAGE plpgsql
    AS $$ begin update users set name = edit_users_name.name where users.id = edit_users_name.id; end; $$;


ALTER PROCEDURE flashback.rename_user(IN id integer, IN name character varying) OWNER TO flashback;

--
-- Name: reorder_block(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_block(IN card_id integer, IN block_position integer, IN target_position integer)
    LANGUAGE plpgsql
    AS $$
declare temporary_position integer = -1;
declare safe_margin integer;
begin
    if block_position <> target_position then
        update blocks set position = temporary_position where card = card_id and position = block_position;

        if target_position < block_position then
            select max(coalesce(position, 0)) + 1 into safe_margin from blocks where card = card_id;
            update blocks set position = position + safe_margin where card = card_id and position >= target_position and position < block_position;
        else
            update blocks set position = position - 1 where card = card_id and position <= target_position and position > block_position;
        end if;

        update blocks set position = target_position where card = card_id and position = temporary_position;

        if target_position < block_position then
            update blocks set position = position - safe_margin + target_position where card = card_id and position >= safe_margin;
        end if;
    end if;
end; $$;


ALTER PROCEDURE flashback.reorder_block(IN card_id integer, IN block_position integer, IN target_position integer) OWNER TO flashback;

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

CREATE PROCEDURE flashback.reorder_milestone(IN roadmap_id integer, IN old_position integer, IN new_position integer)
    LANGUAGE plpgsql
    AS $$
declare top_position integer;
begin
    -- locate the free position on top
    select coalesce(max(m.position), 0) + 1 into top_position from milestones m where m.roadmap = roadmap_id;

    -- relocate targeted milestone to the top
    update milestones m set position = top_position where m.roadmap = roadmap_id and m.position = old_position;

    -- relocate milestones between the old and new positions upwards or downwards to open up a space for targeted milestone on the new location
    if new_position < old_position then
        update milestones m set position = m.position + 1 where m.roadmap = roadmap_id and m.position >= new_position and m.position < old_position;
    else
        update milestones m set position = m.position - 1 where m.roadmap = roadmap_id and m.position > old_position and m.position <= new_position;
    end if;

    -- relocate targeted milestone from top to the new free location
    update milestones m set position = new_position where m.roadmap = roadmap_id and m.position = top_position;

    -- normalize milestone positions to avoid gaps
    update milestones m set position = sub.correction from (
        select row_number() over (order by im.position) as correction, im.position from milestones im where im.roadmap = roadmap_id
    ) sub
    where m.roadmap = roadmap_id and m.position = sub.position;
end;
$$;


ALTER PROCEDURE flashback.reorder_milestone(IN roadmap_id integer, IN old_position integer, IN new_position integer) OWNER TO flashback;

--
-- Name: reorder_section(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_section(IN resource_id integer, IN section_position integer, IN target_position integer)
    LANGUAGE plpgsql
    AS $$
declare temporary_position integer = -1;
declare safe_margin integer;
begin
    if section_position <> target_position then
        update sections set position = temporary_position where resource = resource_id and position = section_position;

        if target_position < section_position then
            select max(coalesce(position, 0)) + 1 into safe_margin from sections where resource = resource_id;
            update sections set position = position + safe_margin where resource = resource_id and position >= target_position and position < section_position;
        else
            update sections set position = position - 1 where resource = resource_id and position <= target_position and position > section_position;
        end if;

        update sections set position = target_position where resource = resource_id and position = temporary_position;

        if target_position < section_position then
            update sections set position = position - safe_margin + target_position where resource = resource_id and position >= safe_margin;
        end if;
    end if;
end; $$;


ALTER PROCEDURE flashback.reorder_section(IN resource_id integer, IN section_position integer, IN target_position integer) OWNER TO flashback;

--
-- Name: reorder_topic(integer, flashback.expertise_level, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.reorder_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN target_position integer)
    LANGUAGE plpgsql
    AS $$
declare temporary_position integer = -1;
declare safe_margin integer;
begin
    if topic_position <> target_position then
        update topics set position = temporary_position where subject = subject_id and level = topic_level and position = topic_position;

        if target_position < topic_position then
            select max(coalesce(position, 0)) + 1 into safe_margin from topics where subject = subject_id and level = topic_level;
            update topics set position = position + safe_margin where subject = subject_id and level = topic_level and position >= target_position and position < topic_position;
        else
            update topics set position = position - 1 where subject = subject_id and level = topic_level and position <= target_position and position > topic_position;
        end if;

        update topics set position = target_position where subject = subject_id and level = topic_level and position = temporary_position;

        if target_position < topic_position then
            update topics set position = position - safe_margin + target_position where subject = subject_id and level = topic_level and position >= safe_margin;
        end if;
    end if;
end; $$;


ALTER PROCEDURE flashback.reorder_topic(IN subject_id integer, IN topic_level flashback.expertise_level, IN topic_position integer, IN target_position integer) OWNER TO flashback;

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
-- Name: search_cards(integer, flashback.expertise_level, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_cards(subject_id integer, milestone_level flashback.expertise_level, search_pattern character varying) RETURNS TABLE(similarity bigint, id integer, state flashback.card_state, headline flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.1;

    return query
    select row_number() over (order by c.headline <-> search_pattern), c.id, c.state, c.headline
    from topic_cards t
    join cards c on c.id = t.card
    where t.subject = subject_id and t.level <= milestone_level and c.headline % search_pattern
    limit 50;
end;
$$;


ALTER FUNCTION flashback.search_cards(subject_id integer, milestone_level flashback.expertise_level, search_pattern character varying) OWNER TO flashback;

--
-- Name: search_presenters(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_presenters(token character varying) RETURNS TABLE(similarity bigint, presenter integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.1;

    return query
    select row_number() over (order by p.name <-> token), p.id, p.name
    from presenters p
    where p.name % token
    limit 5;
end; $$;


ALTER FUNCTION flashback.search_presenters(token character varying) OWNER TO flashback;

--
-- Name: search_providers(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_providers(token character varying) RETURNS TABLE(similarity bigint, provider integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.1;

    return query
    select row_number() over (order by p.name <-> token), p.id, p.name
    from providers p
    where p.name % token and p.name <> 'Flashback'
    limit 5;
end; $$;


ALTER FUNCTION flashback.search_providers(token character varying) OWNER TO flashback;

--
-- Name: search_resources(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_resources(search_pattern character varying) RETURNS TABLE(similarity bigint, id integer, name flashback.citext, type flashback.resource_type, pattern flashback.section_pattern, link character varying, production integer, expiration integer)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.11;

    return query
    select row_number() over (order by r.name <-> search_pattern, r.production desc, r.expiration desc), r.id, r.name, r.type, r.pattern, r.link, r.production, r.expiration
    from resources r
    where r.name % search_pattern
    limit 20;
end;
$$;


ALTER FUNCTION flashback.search_resources(search_pattern character varying) OWNER TO flashback;

--
-- Name: search_roadmaps(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_roadmaps(token character varying) RETURNS TABLE(similarity bigint, roadmap integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.1;

    return query
    select row_number() over (order by r.name <-> token), r.id, r.name
    from roadmaps r
    where r.name % token
    limit 5;
end; $$;


ALTER FUNCTION flashback.search_roadmaps(token character varying) OWNER TO flashback;

--
-- Name: search_sections(integer, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_sections(resource_id integer, search_pattern character varying) RETURNS TABLE(similarity bigint, "position" integer, name flashback.citext, link character varying)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.11;

    return query select row_number() over (order by s.name <-> search_pattern, s.position), s.position, s.name, s.link from sections s where s.resource = resource_id and s.name % search_pattern limit 5;
end; $$;


ALTER FUNCTION flashback.search_sections(resource_id integer, search_pattern character varying) OWNER TO flashback;

--
-- Name: search_subjects(character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_subjects(token character varying) RETURNS TABLE(similarity bigint, id integer, name flashback.citext)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.1;

    return query select row_number() over (order by s.name <-> token), s.id, s.name from subjects s where s.name % token limit 10;
end $$;


ALTER FUNCTION flashback.search_subjects(token character varying) OWNER TO flashback;

--
-- Name: search_topics(integer, flashback.expertise_level, character varying); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.search_topics(subject_id integer, topic_level flashback.expertise_level, search_pattern character varying) RETURNS TABLE(similarity bigint, "position" integer, name flashback.citext, level flashback.expertise_level)
    LANGUAGE plpgsql
    AS $$
begin
    set pg_trgm.similarity_threshold = 0.11;

    return query select row_number() over (order by t.name <-> search_pattern, t.position), t.position, t.name, t.level from topics t where t.subject = subject_id and t.level = topic_level and t.name % search_pattern limit 5;
end; $$;


ALTER FUNCTION flashback.search_topics(subject_id integer, topic_level flashback.expertise_level, search_pattern character varying) OWNER TO flashback;

--
-- Name: split_block(integer, integer); Type: FUNCTION; Schema: flashback; Owner: flashback
--

CREATE FUNCTION flashback.split_block(card_id integer, block_position integer) RETURNS TABLE("position" integer, type flashback.content_type, extension character varying, metadata character varying, content text)
    LANGUAGE plpgsql
    AS $$
declare parts_count integer;
declare block_type content_type;
declare block_extension varchar(20);
declare margin integer;
begin
    create temp table block_parts as
    select row_number() over () - 1 + block_position as position, part
    from (
        select regexp_split_to_table(b.content, E'\n\n\n+') as part from blocks b where b.card = card_id and b.position = block_position
    );

    select count(b.position) into parts_count from block_parts b;

    select coalesce(max(b.position), 1) - block_position into margin from blocks b where b.card = card_id;

    select b.type, b.extension into block_type, block_extension from blocks b where b.card = card_id and b.position = block_position;

    update blocks b set position = b.position + margin where b.card = card_id and b.position > block_position;

    --update blocks b set position = new_position from (
    --    select row_number() over (order by position) - 1 + position + parts_count as new_position, position as old_position
    --    from blocks bb where bb.card = card_id and bb.position > block_position
    --) where b.card = card_id and b.position = old_position;

    delete from blocks b where b.card = card_id and b.position = block_position;

    insert into blocks (card, position, content, type, extension) select card_id, block_parts.position, part, block_type, block_extension from block_parts;

    update blocks b set position = b.position - margin + parts_count - 1 where b.card = card_id and b.position > block_position + margin;

    drop table block_parts;

    return query select b.position, b.type, b.extension, b.metadata, b.content from blocks b where b.card = card_id and b.position between block_position and block_position + parts_count - 1;
end; $$;


ALTER FUNCTION flashback.split_block(card_id integer, block_position integer) OWNER TO flashback;

--
-- Name: study(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.study(IN user_id integer, IN card_id integer, IN time_duration integer)
    LANGUAGE plpgsql
    AS $$
begin
    insert into studies ("user", card, duration)
    values (user_id, card_id, time_duration)
    on conflict on constraint studies_pkey
    do update set
        duration = time_duration,
        last_study = now()
    where studies."user" = user_id and studies.card = card_id;
end;
$$;


ALTER PROCEDURE flashback.study(IN user_id integer, IN card_id integer, IN time_duration integer) OWNER TO flashback;

--
-- Name: throw_back_progress(integer, integer, integer); Type: PROCEDURE; Schema: flashback; Owner: flashback
--

CREATE PROCEDURE flashback.throw_back_progress(IN user_id integer, IN card_id integer, IN days integer)
    LANGUAGE plpgsql
    AS $$
begin
    update progress set last_practice = now() - (days * '1 day'::interval) where "user" = user_id and card = card_id;
end; $$;


ALTER PROCEDURE flashback.throw_back_progress(IN user_id integer, IN card_id integer, IN days integer) OWNER TO flashback;

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
    content text NOT NULL,
    type flashback.content_type NOT NULL,
    extension character varying(20) NOT NULL,
    metadata character varying(100) DEFAULT NULL::character varying
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
    headline flashback.citext,
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
-- Name: nerves; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.nerves (
    "user" integer NOT NULL,
    resource integer NOT NULL,
    subject integer NOT NULL
);


ALTER TABLE flashback.nerves OWNER TO flashback;

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
    name flashback.citext NOT NULL
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
-- Name: producers; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.producers (
    resource integer NOT NULL,
    provider integer NOT NULL
);


ALTER TABLE flashback.producers OWNER TO flashback;

--
-- Name: progress; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.progress (
    "user" integer NOT NULL,
    card integer NOT NULL,
    last_practice timestamp with time zone DEFAULT now() NOT NULL,
    duration integer NOT NULL,
    progression integer DEFAULT 0 NOT NULL
);


ALTER TABLE flashback.progress OWNER TO flashback;

--
-- Name: providers; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.providers (
    id integer NOT NULL,
    name flashback.citext NOT NULL
);


ALTER TABLE flashback.providers OWNER TO flashback;

--
-- Name: providers_id_seq; Type: SEQUENCE; Schema: flashback; Owner: flashback
--

ALTER TABLE flashback.providers ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME flashback.providers_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: requirements; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.requirements (
    roadmap integer NOT NULL,
    subject integer NOT NULL,
    level flashback.expertise_level NOT NULL,
    required_subject integer NOT NULL,
    minimum_level flashback.expertise_level
);


ALTER TABLE flashback.requirements OWNER TO flashback;

--
-- Name: resources; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.resources (
    id integer NOT NULL,
    name flashback.citext NOT NULL,
    type flashback.resource_type NOT NULL,
    pattern flashback.section_pattern NOT NULL,
    link character varying(2000),
    production integer NOT NULL,
    expiration integer NOT NULL
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
-- Name: roadmap_id; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.roadmap_id (
    "coalesce" integer
);


ALTER TABLE flashback.roadmap_id OWNER TO flashback;

--
-- Name: roadmaps; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.roadmaps (
    id integer NOT NULL,
    name flashback.citext NOT NULL,
    "user" integer NOT NULL
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
-- Name: section_cards; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.section_cards (
    resource integer NOT NULL,
    section integer NOT NULL,
    card integer NOT NULL,
    "position" integer NOT NULL
);


ALTER TABLE flashback.section_cards OWNER TO flashback;

--
-- Name: sections; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.sections (
    resource integer NOT NULL,
    "position" integer NOT NULL,
    name flashback.citext,
    link character varying(2000),
    state flashback.closure_state DEFAULT 'draft'::flashback.closure_state NOT NULL
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
-- Name: sessions; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.sessions (
    "user" integer NOT NULL,
    token character varying(300) NOT NULL,
    device character varying(50) NOT NULL,
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
-- Name: studies; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.studies (
    "user" integer NOT NULL,
    card integer NOT NULL,
    last_study timestamp with time zone DEFAULT now() NOT NULL,
    duration integer NOT NULL
);


ALTER TABLE flashback.studies OWNER TO flashback;

--
-- Name: subjects; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.subjects (
    id integer NOT NULL,
    name flashback.citext NOT NULL
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
-- Name: topic_cards; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.topic_cards (
    topic integer NOT NULL,
    card integer NOT NULL,
    "position" integer NOT NULL,
    subject integer NOT NULL,
    level flashback.expertise_level NOT NULL
);


ALTER TABLE flashback.topic_cards OWNER TO flashback;

--
-- Name: topics; Type: TABLE; Schema: flashback; Owner: flashback
--

CREATE TABLE flashback.topics (
    "position" integer NOT NULL,
    name flashback.citext NOT NULL,
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
-- Name: assessments assessments_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.assessments
    ADD CONSTRAINT assessments_pkey PRIMARY KEY (subject, level, topic, card);


--
-- Name: authors authors_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.authors
    ADD CONSTRAINT authors_pkey PRIMARY KEY (resource, presenter);


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
-- Name: milestones milestones_roadmap_subject_position_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.milestones
    ADD CONSTRAINT milestones_roadmap_subject_position_key UNIQUE (roadmap, subject, "position");


--
-- Name: nerves nerves_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.nerves
    ADD CONSTRAINT nerves_pkey PRIMARY KEY ("user", resource, subject);


--
-- Name: network_activities network_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.network_activities
    ADD CONSTRAINT network_activities_pkey PRIMARY KEY (id);


--
-- Name: presenters presenters_name_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.presenters
    ADD CONSTRAINT presenters_name_key UNIQUE (name);


--
-- Name: presenters presenters_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.presenters
    ADD CONSTRAINT presenters_pkey PRIMARY KEY (id);


--
-- Name: producers producers_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.producers
    ADD CONSTRAINT producers_pkey PRIMARY KEY (resource, provider);


--
-- Name: progress progress_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.progress
    ADD CONSTRAINT progress_pkey PRIMARY KEY ("user", card);


--
-- Name: providers providers_name_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.providers
    ADD CONSTRAINT providers_name_key UNIQUE (name);


--
-- Name: providers providers_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.providers
    ADD CONSTRAINT providers_pkey PRIMARY KEY (id);


--
-- Name: requirements requirements_roadmap_subject_level_required_subject_minimum_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.requirements
    ADD CONSTRAINT requirements_roadmap_subject_level_required_subject_minimum_key UNIQUE (roadmap, subject, level, required_subject, minimum_level);


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
-- Name: roadmaps roadmaps_user_name_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.roadmaps
    ADD CONSTRAINT roadmaps_user_name_key UNIQUE ("user", name);


--
-- Name: section_cards section_cards_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.section_cards
    ADD CONSTRAINT section_cards_pkey PRIMARY KEY (resource, section, card);


--
-- Name: section_cards section_cards_unique_position_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.section_cards
    ADD CONSTRAINT section_cards_unique_position_key UNIQUE (resource, section, "position");


--
-- Name: sections_activities sections_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections_activities
    ADD CONSTRAINT sections_activities_pkey PRIMARY KEY (id);


--
-- Name: sections sections_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sections
    ADD CONSTRAINT sections_pkey PRIMARY KEY (resource, "position");


--
-- Name: sessions sessions_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.sessions
    ADD CONSTRAINT sessions_pkey PRIMARY KEY (token, device, "user");


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
-- Name: studies studies_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.studies
    ADD CONSTRAINT studies_pkey PRIMARY KEY ("user", card);


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
-- Name: topic_cards topic_cards_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topic_cards
    ADD CONSTRAINT topic_cards_pkey PRIMARY KEY (subject, level, topic, card);


--
-- Name: topic_cards topic_cards_unique_card_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topic_cards
    ADD CONSTRAINT topic_cards_unique_card_key UNIQUE (subject, card);


--
-- Name: topic_cards topic_cards_unique_position_key; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topic_cards
    ADD CONSTRAINT topic_cards_unique_position_key UNIQUE (subject, topic, level, "position");


--
-- Name: topics_activities topics_activities_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics_activities
    ADD CONSTRAINT topics_activities_pkey PRIMARY KEY (id);


--
-- Name: topics topics_pkey; Type: CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topics
    ADD CONSTRAINT topics_pkey PRIMARY KEY (subject, level, "position");


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
-- Name: cards_headline_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX cards_headline_trigram ON flashback.cards USING gin (headline flashback.gin_trgm_ops);


--
-- Name: presenters_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX presenters_name_trigram ON flashback.presenters USING gin (name flashback.gin_trgm_ops);


--
-- Name: providers_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX providers_name_trigram ON flashback.providers USING gin (name flashback.gin_trgm_ops);


--
-- Name: resource_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX resource_name_trigram ON flashback.resources USING gin (name flashback.gin_trgm_ops);


--
-- Name: roadmaps_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX roadmaps_name_trigram ON flashback.roadmaps USING gin (name flashback.gin_trgm_ops);


--
-- Name: sections_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX sections_name_trigram ON flashback.sections USING gin (name flashback.gin_trgm_ops);


--
-- Name: subjects_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX subjects_name_trigram ON flashback.subjects USING gin (name flashback.gin_trgm_ops);


--
-- Name: topics_name_trigram; Type: INDEX; Schema: flashback; Owner: flashback
--

CREATE INDEX topics_name_trigram ON flashback.topics USING gin (name flashback.gin_trgm_ops);


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
-- Name: nerves nerves_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.nerves
    ADD CONSTRAINT nerves_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: nerves nerves_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.nerves
    ADD CONSTRAINT nerves_subject_fkey FOREIGN KEY (subject) REFERENCES flashback.subjects(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: nerves nerves_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.nerves
    ADD CONSTRAINT nerves_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: network_activities network_activities_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.network_activities
    ADD CONSTRAINT network_activities_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE;


--
-- Name: producers producers_provider_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.producers
    ADD CONSTRAINT producers_provider_fkey FOREIGN KEY (provider) REFERENCES flashback.providers(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: producers producers_resource_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.producers
    ADD CONSTRAINT producers_resource_fkey FOREIGN KEY (resource) REFERENCES flashback.resources(id) ON UPDATE CASCADE ON DELETE CASCADE;


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
-- Name: requirements requirements_roadmap_required_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.requirements
    ADD CONSTRAINT requirements_roadmap_required_subject_fkey FOREIGN KEY (roadmap, required_subject) REFERENCES flashback.milestones(roadmap, subject) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: requirements requirements_roadmap_subject_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.requirements
    ADD CONSTRAINT requirements_roadmap_subject_fkey FOREIGN KEY (roadmap, subject) REFERENCES flashback.milestones(roadmap, subject) ON UPDATE CASCADE ON DELETE CASCADE;


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
-- Name: roadmaps roadmaps_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.roadmaps
    ADD CONSTRAINT roadmaps_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: section_cards section_cards_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.section_cards
    ADD CONSTRAINT section_cards_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: section_cards section_cards_resource_position_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.section_cards
    ADD CONSTRAINT section_cards_resource_position_fkey FOREIGN KEY (resource, section) REFERENCES flashback.sections(resource, "position") ON UPDATE CASCADE ON DELETE CASCADE;


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
-- Name: studies studies_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.studies
    ADD CONSTRAINT studies_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: studies studies_user_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.studies
    ADD CONSTRAINT studies_user_fkey FOREIGN KEY ("user") REFERENCES flashback.users(id) ON UPDATE CASCADE ON DELETE CASCADE;


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
-- Name: topic_cards topic_cards_card_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topic_cards
    ADD CONSTRAINT topic_cards_card_fkey FOREIGN KEY (card) REFERENCES flashback.cards(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: topic_cards topic_cards_subject_level_topic_fkey; Type: FK CONSTRAINT; Schema: flashback; Owner: flashback
--

ALTER TABLE ONLY flashback.topic_cards
    ADD CONSTRAINT topic_cards_subject_level_topic_fkey FOREIGN KEY (subject, level, topic) REFERENCES flashback.topics(subject, level, "position") ON UPDATE CASCADE ON DELETE CASCADE;


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
-- Name: SCHEMA public; Type: ACL; Schema: -; Owner: flashback
--

REVOKE USAGE ON SCHEMA public FROM PUBLIC;
GRANT ALL ON SCHEMA public TO flashback_client;


--
-- PostgreSQL database dump complete
--

\unrestrict 8M8TEeWEskUJ4ARsIP8wFWASXdZbdYUNMs9fzOUzo27AggF7iehSikPrUcGKpuh

