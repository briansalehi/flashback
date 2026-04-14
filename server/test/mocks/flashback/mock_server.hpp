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
    MOCK_METHOD(grpc::Status, GetRoadmaps, (grpc:: ServerContext*, GetRoadmapsRequest const*, GetRoadmapsResponse*), (override));
    MOCK_METHOD(grpc::Status, RenameRoadmap, (grpc:: ServerContext*, RenameRoadmapRequest const*, RenameRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, RemoveRoadmap, (grpc:: ServerContext*, RemoveRoadmapRequest const*, RemoveRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, SearchRoadmaps, (grpc:: ServerContext*, SearchRoadmapsRequest const*, SearchRoadmapsResponse*), (override));
    MOCK_METHOD(grpc::Status, CloneRoadmap, (grpc::ServerContext*, CloneRoadmapRequest const*, CloneRoadmapResponse*), (override));
    MOCK_METHOD(grpc::Status, AddMilestone, (grpc::ServerContext*, AddMilestoneRequest const*, AddMilestoneResponse*), (override));
    MOCK_METHOD(grpc::Status, GetMilestones, (grpc::ServerContext*, GetMilestonesRequest const*, GetMilestonesResponse*), (override));
    MOCK_METHOD(grpc::Status, AddRequirement, (grpc::ServerContext *, AddRequirementRequest const*, AddRequirementResponse*), (override));
    MOCK_METHOD(grpc::Status, GetRequirements, (grpc::ServerContext*, GetRequirementsRequest const* , GetRequirementsResponse*), (override));
    MOCK_METHOD(grpc::Status, ReorderMilestone, (grpc::ServerContext*, ReorderMilestoneRequest const*, ReorderMilestoneResponse*), (override));
    MOCK_METHOD(grpc::Status, RemoveMilestone, (grpc::ServerContext*, RemoveMilestoneRequest const*, RemoveMilestoneResponse*), (override));
    MOCK_METHOD(grpc ::Status, ChangeMilestoneLevel, (grpc::ServerContext*, ChangeMilestoneLevelRequest const*, ChangeMilestoneLevelResponse*), (override));
    MOCK_METHOD(grpc::Status, CreateSubject, (grpc::ServerContext*, CreateSubjectRequest const*, CreateSubjectResponse*), (override));
    MOCK_METHOD(grpc::Status, SearchSubjects, (grpc ::ServerContext*, SearchSubjectsRequest const*, SearchSubjectsResponse*), (override));
    MOCK_METHOD(grpc::Status, RenameSubject, (grpc::ServerContext*, const flashback::RenameSubjectRequest*, flashback::RenameSubjectResponse*), (override));
    MOCK_METHOD(grpc::Status, RemoveSubject, (grpc::ServerContext*, RemoveSubjectRequest const*, RemoveSubjectResponse*), (override));
};
} // flashback
