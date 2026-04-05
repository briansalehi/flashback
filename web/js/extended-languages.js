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
