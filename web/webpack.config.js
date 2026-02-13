const path = require('path');

module.exports = {
  mode: 'production',
  entry: './bundle.js',
  output: {
    filename: 'flashback.js',
    path: __dirname,
  },
};
