window.types_pb = require('./types_pb.js');
window.requests_pb = require('./requests_pb.js');
window.server_pb = require('./server_pb.js');
window.server_grpc_web_pb = require('./server_grpc_web_pb.js');

console.log('Raw modules loaded:', {
    types: window.types_pb,
    requests: window.requests_pb,
    server: window.server_pb,
    grpc: window.server_grpc_web_pb
});