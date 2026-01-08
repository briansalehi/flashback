#pragma once

#include <flashback/server.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace flashback
{
class mock_server final: public server
{
public:
    MOCK_METHOD(grpc::Status, SignUp, (grpc::ServerContext*, SignUpRequest const*, SignUpResponse*), (override));
    MOCK_METHOD(grpc::Status, SignIn, (grpc::ServerContext*, SignInRequest const*, SignInResponse*), (override));
    MOCK_METHOD(grpc::Status, VerifySession, (grpc:: ServerContext*, VerifySessionRequest const*, VerifySessionResponse*), (override));
    MOCK_METHOD(grpc::Status, ResetPassword, (grpc:: ServerContext*, ResetPasswordRequest const*, ResetPasswordResponse*), (override));
    MOCK_METHOD(grpc::Status, CreateRoadmap, (grpc:: ServerContext*, CreateRoadmapRequest const*, CreateRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, AssignRoadmap, (grpc:: ServerContext*, AssignRoadmapRequest const*, AssignRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, GetRoadmaps, (grpc:: ServerContext*, GetRoadmapsRequest const*, GetRoadmapsResponse*), (override));
    MOCK_METHOD(grpc::Status, RenameRoadmap, (grpc:: ServerContext*, RenameRoadmapRequest const*, RenameRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, RemoveRoadmap, (grpc:: ServerContext*, RemoveRoadmapRequest const*, RemoveRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, SearchRoadmaps, (grpc:: ServerContext*, SearchRoadmapsRequest const*, SearchRoadmapsResponse*), (override));
};
} // flashback
