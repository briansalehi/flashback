#pragma once

#include <memory>
#include <types.pb.h>
#include <server.grpc.pb.h>
#include <flashback/database.hpp>

namespace flashback
{
class server: public Server::Service
{
public:
    explicit server(std::shared_ptr<basic_database> database);
    ~server() override = default;

    // entry page
    grpc::Status SignUp(grpc::ServerContext* context, SignUpRequest const* request, SignUpResponse* response) override;
    grpc::Status SignIn(grpc::ServerContext* context, SignInRequest const* request, SignInResponse* response) override;
    grpc::Status VerifySession(grpc::ServerContext* context, VerifySessionRequest const* request, VerifySessionResponse* response) override;
    // VerifyUser

    // accounts page
    grpc::Status ResetPassword(grpc::ServerContext* context, ResetPasswordRequest const* request, ResetPasswordResponse* response) override;
    // RenameUser
    // ChangeUserEmail

    // home page
    grpc::Status GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response) override;
    // GetStudyResources

    // roadmap page
    grpc::Status CreateRoadmap(grpc::ServerContext* context, CreateRoadmapRequest const* request, CreateRoadmapResponse* response) override;
    grpc::Status RenameRoadmap(grpc::ServerContext* context, RenameRoadmapRequest const* request, RenameRoadmapResponse* response) override;
    grpc::Status RemoveRoadmap(grpc::ServerContext* context, RemoveRoadmapRequest const* request, RemoveRoadmapResponse* response) override;
    grpc::Status SearchRoadmaps(grpc::ServerContext* context, SearchRoadmapsRequest const* request, SearchRoadmapsResponse* response) override;
    grpc::Status CloneRoadmap(grpc::ServerContext* context, CloneRoadmapRequest const* request, CloneRoadmapResponse* response) override;

    // milestone page
    grpc::Status GetMilestones(grpc::ServerContext* context, GetMilestonesRequest const* request, GetMilestonesResponse* response) override;
    grpc::Status AddMilestone(grpc::ServerContext* context, AddMilestoneRequest const* request, AddMilestoneResponse* response) override;
    grpc::Status AddRequirement(grpc::ServerContext* context, AddRequirementRequest const* request, AddRequirementResponse* response) override;
    grpc::Status GetRequirements(grpc::ServerContext* context, GetRequirementsRequest const* request, GetRequirementsResponse* response) override;
    grpc::Status ReorderMilestone(grpc::ServerContext* context, ReorderMilestoneRequest const* request, ReorderMilestoneResponse* response) override;
    grpc::Status RemoveMilestone(grpc::ServerContext* context, RemoveMilestoneRequest const* request, RemoveMilestoneResponse* response) override;
    grpc::Status ChangeMilestoneLevel(grpc::ServerContext* context, ChangeMilestoneLevelRequest const* request, ChangeMilestoneLevelResponse* response) override;

    // subject page
    grpc::Status CreateSubject(grpc::ServerContext* context, CreateSubjectRequest const* request, CreateSubjectResponse* response) override;
    grpc::Status SearchSubjects(grpc::ServerContext* context, SearchSubjectsRequest const* request, SearchSubjectsResponse* response) override;
    grpc::Status RenameSubject(grpc::ServerContext* context, RenameSubjectRequest const* request, RenameSubjectResponse* response) override;
    grpc::Status RemoveSubject(grpc::ServerContext* context, RemoveSubjectRequest const* request, RemoveSubjectResponse* response) override;
    grpc::Status MergeSubjects(grpc::ServerContext* context, MergeSubjectsRequest const* request, MergeSubjectsResponse* response) override;

    // resource page
    // GetResources
    // CreateResource
    // AddResourceToSubject
    // DropResourceFromSubject
    // SearchResources
    // MergeResources
    // RenameResource
    // RemoveResource
    // EditResourceProduction
    // EditResourceExpiration
    // EditResourceLink
    // ChangeResourceType
    // ChangeSectionPattern
    // GetRelevantSubjects

    // nerves
    // CreateNerve
    // GetNerves

    // providers
    // CreateProvider
    // AddProvider
    // DropProvider
    // SearchProviders
    // RenameProvider
    // RemoveProvider
    // MergeProviders

    // presenters
    // CreatePresenter
    // AddPresenter
    // DropPresenter
    // SearchPresenters
    // RenamePresenters
    // RemovePresenters
    // MergePresenters

    // topic page
    // GetTopics
    // CreateTopic
    // ReorderTopic
    // RemoveTopic
    // MergeTopics
    // RenameTopic
    // MoveTopic
    // SearchTopics
    // ChangeTopicLevel

    // section page
    // GetSections
    // CreateSection
    // ReorderSection
    // RemoveSection
    // MergeSections
    // RenameSection
    // MoveSection
    // SearchSections
    // EditSectionLink

    // shared functions in section and topic pages
    // CreateCard
    // AddCardToSection
    // AddCardToTopic
    // ReorderCard
    // RemoveCard
    // MergeCards
    // SearchCards

    // section page
    // GetStudyCards
    // MoveCardToSection
    // MarkSectionAsReviewed

    // topic page
    // GetPracticeCards
    // MoveCardToTopic
    // CreateAssessment
    // ExpandAssessment
    // DiminishAssessment
    // GetAssimilationCoverage
    // IsAssimilated

    // card page
    // EditHeadline
    // CreateBlock
    // GetBlocks
    // EditBlockContent
    // RemoveBlock
    // EditBlockExtension
    // ChangeBlockType
    // EditBlockMetaData
    // ReorderBlock
    // MergeBlocks
    // SplitBlock
    // MarkCardAsReviewed
    // GetVariations

    // progress
    // Study
    // MakeProgress
    // GetProgressWeight

protected:
    [[nodiscard]] static std::string calculate_hash(std::string_view password);
    [[nodiscard]] static bool password_is_valid(std::string_view lhs, std::string_view rhs);
    [[nodiscard]] static std::string generate_token();
    [[nodiscard]] bool session_is_valid(User const& user) const;

    std::shared_ptr<basic_database> m_database;
};
} // flashback
