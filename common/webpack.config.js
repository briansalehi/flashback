const path = require('path');

module.exports = {
  mode: 'production',
  entry: ['./types_pb.js', './requests_pb.js', './server_pb.js', './server_grpc_web_pb.js'],
  output: {
    filename: 'flashback.js',
    path: __dirname,
  },
};
