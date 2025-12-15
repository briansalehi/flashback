#pragma once

#include <memory>
#include <types.pb.h>
#include <server.grpc.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class server final: public Server::Service
{
public:
    explicit server(std::shared_ptr<database> database);
    ~server() override = default;

    // entry page requests
    grpc::Status SignUp(grpc::ServerContext* context, SignUpRequest const* request, SignUpResponse* response) override;
    grpc::Status SignIn(grpc::ServerContext* context, SignInRequest const* request, SignInResponse* response) override;
    grpc::Status VerifySession(grpc::ServerContext* context, VerifySessionRequest const* request, VerifySessionResponse* response) override;
    grpc::Status ResetPassword(grpc::ServerContext* context, ResetPasswordRequest const* request, ResetPasswordResponse* response) override;
    // RenameUser
    // ChangeUserEmail
    // VerifyUser
    // DeactivateUser

    // welcome page requests
    grpc::Status GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response) override;
    // CreateRoadmap
    // ReorderRoadmap
    // RemoveRoadmap

    // roadmap page requests
    // RenameRoadmap
    // GetMilestones
    // AddMilestone
    // ReorderMilestone
    // RemoveMilestone
    // ChangeMilestoneLevel

    //

private:
    [[nodiscard]] static std::string calculate_hash(std::string_view password);
    [[nodiscard]] static bool password_is_valid(std::string_view lhs, std::string_view rhs);
    [[nodiscard]] static std::string generate_token();
    [[nodiscard]] bool session_is_valid(std::string_view email, std::string_view device, std::string_view token) const;

    std::shared_ptr<database> m_database;
};
} // flashback
