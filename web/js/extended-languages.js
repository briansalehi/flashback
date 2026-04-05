// sudoers, sudo, visudo syntax
Prism.languages.sudoers = {
    'comment': { pattern: /#.*/, greedy: true },
    'alias-declaration': {
        pattern: /\b((?:User|Cmnd|Host|Runas)_Alias\s+)\w+/,
        lookbehind: true,
        alias: 'function'
    },
    'keyword': /\b(?:User_Alias|Cmnd_Alias|Host_Alias|Runas_Alias)\b/,
    'tag': {
        pattern: /\b(?:NOPASSWD|PASSWD|NOEXEC|EXEC|SETENV|NOSETENV|LOG_INPUT|NOLOG_INPUT|LOG_OUTPUT|NOLOG_OUTPUT)(?=\s*:)/
    },

    // These must come BEFORE 'important' so ALL in hostname position wins
    'username': {
        pattern: /^[%+]?\w+(?=\s+\S+\s*=)/m,
        alias: 'symbol'
    },
    'hostname': {
        pattern: /(?<=^[%+]?\w+\s+)[^\s=,\[\]]+(?:\[[^\]]*\])?[^\s=,]*(?=\s*=)/m,
        alias: 'builtin'
    },

    'important': /\bALL\b/,
    'runas': {
        pattern: /\([^)]*\)/,
        inside: {
            'punctuation': /[():]/,
            'important': /\bALL\b/,
            'variable': /\w+/
        }
    },
    'command': { pattern: /\/(?:[^\s,:()\[\]\\]|\\.)+/, alias: 'string' },
    'operator': /[=!]/,
    'punctuation': /[,:()\[\]]/,
};
Prism.languages.sudo = Prism.languages.sudoers;
Prism.languages.visudo = Prism.languages.sudoers;

// postgresql, postgres, pgsql, psql syntax
Prism.languages.pgsql = Prism.languages.extend('sql', {
    'comment': {
        pattern: /(^|[^\\])(?:\/\*[\s\S]*?\*\/|--.*)/,
        lookbehind: true
    },

    // Must be first and greedy — claims $$ blocks before anything else runs
    'dollar-string': {
        pattern: /(\$(?:[^$\s]*)\$)[\s\S]*?\1/,
        greedy: true,
        inside: {
            'delimiter': {
                pattern: /^\$[^$]*\$|\$[^$]*\$$/,
                alias: 'punctuation'
            },
            'plpgsql-body': {
                pattern: /[\s\S]+/,
                inside: null  // wired up after definition
            }
        }
    },

    'string': {
        pattern: /(^|[^\\])(')(?:\\[\s\S]|(?!\2)[^\\]|\2\2)*\2/,
        greedy: true,
        lookbehind: true
    },

    'identifier': {
        pattern: /"(?:\\[\s\S]|[^"\\]|"")*"/,
        greedy: true,
        inside: { punctuation: /^"|"$/ }
    },

    'parameter': {
        pattern: /\$\d+/,
        alias: 'variable'
    },

    'typecast': {
        pattern: /::/,
        alias: 'operator'
    },

    // Block structure keywords — before 'keyword' so they win
    'block-keyword': {
        pattern: /\b(?:BEGIN|END|LOOP|THEN|ELSE|ELSIF|ELSEIF)\b/i,
        alias: 'important'
    },

    // PL/pgSQL-specific — before 'keyword' so QUERY etc. don't get missed
    'plpgsql-keyword': {
        pattern: /\b(?:ALIAS|ASSERT|CLOSE|CONSTANT|CONTINUE|DIAGNOSTICS|ELSEIF|ELSIF|ERROR|EXCEPTION|EXIT|FOREACH|FOUND|GET|HINT|INFO|LOG|NOTICE|OPEN|PERFORM|QUERY|RECORD|RETURN|REVERSE|ROWTYPE|SLICE|SQLSTATE|STRICT|TYPE|WARNING)\b/i,
        alias: 'keyword'
    },

    'function': /\b(?:AVG|COUNT|MIN|MAX|SUM|COALESCE|NULLIF|GREATEST|LEAST|NOW|CURRENT_TIMESTAMP|CURRENT_DATE|DATE_TRUNC|DATE_PART|EXTRACT|AGE|TO_CHAR|TO_DATE|TO_TIMESTAMP|STRING_AGG|ARRAY_AGG|JSON_AGG|JSONB_AGG|ROW_TO_JSON|UNNEST|GENERATE_SERIES|FORMAT|QUOTE_IDENT|QUOTE_LITERAL)(?=\s*\()/i,

    'keyword': /\b(?:ACTION|ADD|ALL|ALTER|ANALYZE|AND|ANY|ARRAY|AS|ASC|BY|CALL|CASCADE|CASE|CHECK|COLUMN|COMMIT|CONSTRAINT|CREATE|CROSS|CURRENT(?:_DATE|_TIME|_TIMESTAMP|_USER)?|CURSOR|DATA(?:BASE)?|DATE|DECLARE|DEFAULT|DELETE|DESC|DISTINCT|DO|DROP|EXCEPT|EXISTS|EXPLAIN|FETCH|FILTER|FOR|FOREIGN|FROM|FULL|FUNCTION|GRANT|GROUP|HAVING|IF|ILIKE|IN|INDEX|INNER|INSERT|INTERSECT|INTO|IS|JOIN|LANGUAGE|LATERAL|LEFT|LIKE|LIMIT|LOCK|MATCH|MATERIALIZED|NATURAL|NEXT|NOT|NOTIFY|NULL|OF|OFFSET|ON|ONLY|OR|ORDER|OUTER|OVER|PARTITION|PLACING|PREPARE|PRIMARY|PROCEDURE|REFERENCES|RELEASE|RENAME|REPLACE|RETURNING|REVOKE|RIGHT|ROLLBACK|ROW|ROWS|RULE|SAVEPOINT|SCHEMA|SELECT|SEQUENCE|SESSION(?:_USER)?|SET|SIMILAR|SOME|START|TABLE|TEMP(?:ORARY)?|TO|TRANSACTION|TRIGGER|TRUNCATE|UNION|UNIQUE|UPDATE|USING|VACUUM|VALUES|VIEW|VOLATILE|WHEN|WHERE|WITH|WORK)\b/i,

    'type-multiword': {
        pattern: /\b(?:bit varying|character varying|double precision|timestamp\s+(?:with|without)\s+time\s+zone|time\s+(?:with|without)\s+time\s+zone)\b/i,
        alias: 'builtin'
    },
    'type': {
        pattern: /\b(?:BIGINT|BIGSERIAL|BIT|BOOL|BOOLEAN|BOX|BYTEA|CIDR|CIRCLE|DATE|FLOAT4|FLOAT8|HSTORE|INET|INT|INT2|INT4|INT8|INTEGER|INTERVAL|JSON|JSONB|LINE|LSEG|MACADDR|MONEY|NUMERIC|OID|POINT|REAL|REGCLASS|SERIAL|SERIAL2|SERIAL4|SERIAL8|SMALLINT|SMALLSERIAL|TEXT|TIME|TIMESTAMP|TIMESTAMPTZ|TIMETZ|TSQUERY|TSVECTOR|UUID|VARCHAR|CHAR|XML)\b/i,
        alias: 'builtin'
    },

    'boolean': /\b(?:TRUE|FALSE|NULL)\b/i,
    'number': /\b0x[\da-f]+\b|\b\d+(?:\.\d*)?|\B\.\d+\b/i,
    'operator': /[-+*\/=%^~]|&&?|\|\|?|!=?|<(?:=>?|<|>)?|>[>=]?|\b(?:AND|BETWEEN|IN|IS|ILIKE|LIKE|NOT|OR|SIMILAR TO)\b/i,
    'punctuation': /[;[\]()`,.]/
});

// Wire up recursive inner grammar
Prism.languages.pgsql['dollar-string'].inside['plpgsql-body'].inside = Prism.languages.pgsql;
Prism.languages.psql = Prism.languages.pgsql;
Prism.languages.postgres = Prism.languages.pgsql;
Prism.languages.postgresql = Prism.languages.pgsql;
