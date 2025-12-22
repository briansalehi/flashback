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
    explicit server(std::shared_ptr<basic_database> database);
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
    grpc::Status CreateRoadmap(grpc::ServerContext* context, CreateRoadmapRequest const* request, CreateRoadmapResponse* response) override;
    grpc::Status AssignRoadmap(grpc::ServerContext* context, AssignRoadmapRequest const* request, AssignRoadmapResponse* response) override;
    grpc::Status GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response) override;
    grpc::Status RenameRoadmap(grpc::ServerContext* context, RenameRoadmapRequest const* request, RenameRoadmapResponse* response) override;
    grpc::Status RemoveRoadmap(grpc::ServerContext* context, RemoveRoadmapRequest const* request, RemoveRoadmapResponse* response) override;
    grpc::Status SearchRoadmaps(grpc::ServerContext* context, SearchRoadmapsRequest const* request, SearchRoadmapsResponse* response) override;

    // roadmap page requests
    // GetMilestones
    // AddMilestone
    // ReorderMilestone
    // RemoveMilestone
    // ChangeMilestoneLevel

    // subject page requests
    // RenameSubject
    // RemoveSubject
    // MergeSubjects
    // GetResources
    // AddResourceToSubject
    // CreateResource
    // DropResourceFromSubject
    // MergeResources
    // GetTopics
    // CreateTopic
    // ReorderTopic
    // RemoveTopic
    // MergeTopics
    // RenameTopic
    // MoveTopic

    // resource page requests
    // GetSections
    // RenameResource
    // RemoveResource
    // EditResourceProvider
    // EditResourcePresenter
    // EditResourceLink
    // ChangeResourceType
    // ChangeSectionPattern
    // ChangeCondition
    // CreateSection
    // ReorderSection
    // RemoveSection
    // MergeSections
    // RenameSection
    // MoveSection

    // section/topic page
    // CreateCard
    // ReorderCard
    // RemoveCard
    // MergeCards

    // section page
    // GetStudyCards
    // RenameSection
    // RemoveSection
    // MoveCardToSection

    // topic page
    // GetPracticeCards
    // RenameTopic
    // RemoveTopic
    // MoveCardToTopic

    // card page
    // EditHeadline
    // CreateBlock
    // EditBlock
    // RemoveBlock
    // EditBlockExtension
    // EditBlockType
    // EditBlockMetaData
    // ReorderBlock
    // MergeBlocks
    // SplitBlock

protected:
    [[nodiscard]] static std::string calculate_hash(std::string_view password);
    [[nodiscard]] static bool password_is_valid(std::string_view lhs, std::string_view rhs);
    [[nodiscard]] static std::string generate_token();
    [[nodiscard]] static bool session_is_valid(grpc::ServerContext* context, std::string_view user_provided_token);

    std::shared_ptr<basic_database> m_database;
};
} // flashback
