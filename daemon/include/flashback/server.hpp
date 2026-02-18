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
    grpc::Status SignOut(grpc::ServerContext* context, SignOutRequest const* request, SignOutResponse* response) override;
    grpc::Status VerifySession(grpc::ServerContext* context, VerifySessionRequest const* request, VerifySessionResponse* response) override;
    // grpc::Status VerifyUser(grpc::ServerContext* context, VerifyUserRequest const* request, VerifyUserResponse* response) override;

    // accounts page
    grpc::Status ResetPassword(grpc::ServerContext* context, ResetPasswordRequest const* request, ResetPasswordResponse* response) override;
    grpc::Status EditUser(grpc::ServerContext* context, EditUserRequest const* request, EditUserResponse* response) override;

    // home page
    grpc::Status GetRoadmaps(grpc::ServerContext* context, GetRoadmapsRequest const* request, GetRoadmapsResponse* response) override;
    grpc::Status GetStudyResources(grpc::ServerContext* context, GetStudyResourcesRequest const* request, GetStudyResourcesResponse* response) override;

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
    grpc::Status GetResources(grpc::ServerContext* context, GetResourcesRequest const* request, GetResourcesResponse* response) override;
    grpc::Status CreateResource(grpc::ServerContext* context, CreateResourceRequest const* request, CreateResourceResponse* response) override;
    grpc::Status AddResourceToSubject(grpc::ServerContext* context, AddResourceToSubjectRequest const* request, AddResourceToSubjectResponse* response) override;
    grpc::Status DropResourceFromSubject(grpc::ServerContext* context, DropResourceFromSubjectRequest const* request, DropResourceFromSubjectResponse* response) override;
    grpc::Status SearchResources(grpc::ServerContext* context, SearchResourcesRequest const* request, SearchResourcesResponse* response) override;
    grpc::Status MergeResources(grpc::ServerContext* context, MergeResourcesRequest const* request, MergeResourcesResponse* response) override;
    grpc::Status RemoveResource(grpc::ServerContext* context, RemoveResourceRequest const* request, RemoveResourceResponse* response) override;
    grpc::Status EditResource(grpc::ServerContext* context, EditResourceRequest const* request, EditResourceResponse* response) override;
    // grpc::Status GetRelevantSubjects(grpc::ServerContext* context, GetRelevantSubjectsRequest const* request, GetRelevantSubjectsResponse* response) override;

    // nerves
    grpc::Status CreateNerve(grpc::ServerContext* context, CreateNerveRequest const* request, CreateNerveResponse* response) override;
    grpc::Status GetNerves(grpc::ServerContext* context, GetNervesRequest const* request, GetNervesResponse* response) override;

    // providers
    grpc::Status CreateProvider(grpc::ServerContext* context, CreateProviderRequest const* request, CreateProviderResponse* response) override;
    grpc::Status AddProvider(grpc::ServerContext* context, AddProviderRequest const* request, AddProviderResponse* response) override;
    grpc::Status DropProvider(grpc::ServerContext* context, DropProviderRequest const* request, DropProviderResponse* response) override;
    grpc::Status SearchProviders(grpc::ServerContext* context, SearchProvidersRequest const* request, SearchProvidersResponse* response) override;
    grpc::Status RenameProvider(grpc::ServerContext* context, RenameProviderRequest const* request, RenameProviderResponse* response) override;
    grpc::Status RemoveProvider(grpc::ServerContext* context, RemoveProviderRequest const* request, RemoveProviderResponse* response) override;
    grpc::Status MergeProviders(grpc::ServerContext* context, MergeProvidersRequest const* request, MergeProvidersResponse* response) override;

    // presenters
    grpc::Status CreatePresenter(grpc::ServerContext* context, CreatePresenterRequest const* request, CreatePresenterResponse* response) override;
    grpc::Status AddPresenter(grpc::ServerContext* context, AddPresenterRequest const* request, AddPresenterResponse* response) override;
    grpc::Status DropPresenter(grpc::ServerContext* context, DropPresenterRequest const* request, DropPresenterResponse* response) override;
    grpc::Status SearchPresenters(grpc::ServerContext* context, SearchPresentersRequest const* request, SearchPresentersResponse* response) override;
    grpc::Status RenamePresenter(grpc::ServerContext* context, RenamePresenterRequest const* request, RenamePresenterResponse* response) override;
    grpc::Status RemovePresenter(grpc::ServerContext* context, RemovePresenterRequest const* request, RemovePresenterResponse* response) override;
    grpc::Status MergePresenters(grpc::ServerContext* context, MergePresentersRequest const* request, MergePresentersResponse* response) override;

    // topic page
    grpc::Status GetTopics(grpc::ServerContext* context, GetTopicsRequest const* request, GetTopicsResponse* response) override;
    grpc::Status CreateTopic(grpc::ServerContext* context, CreateTopicRequest const* request, CreateTopicResponse* response) override;
    grpc::Status ReorderTopic(grpc::ServerContext* context, ReorderTopicRequest const* request, ReorderTopicResponse* response) override;
    grpc::Status RemoveTopic(grpc::ServerContext* context, RemoveTopicRequest const* request, RemoveTopicResponse* response) override;
    grpc::Status MergeTopics(grpc::ServerContext* context, MergeTopicsRequest const* request, MergeTopicsResponse* response) override;
    grpc::Status EditTopic(grpc::ServerContext* context, EditTopicRequest const* request, EditTopicResponse* response) override;
    grpc::Status MoveTopic(grpc::ServerContext* context, MoveTopicRequest const* request, MoveTopicResponse* response) override;
    grpc::Status SearchTopics(grpc::ServerContext* context, SearchTopicsRequest const* request, SearchTopicsResponse* response) override;

    // section page
    grpc::Status GetSections(grpc::ServerContext* context, GetSectionsRequest const* request, GetSectionsResponse* response) override;
    grpc::Status CreateSection(grpc::ServerContext* context, CreateSectionRequest const* request, CreateSectionResponse* response) override;
    grpc::Status ReorderSection(grpc::ServerContext* context, ReorderSectionRequest const* request, ReorderSectionResponse* response) override;
    grpc::Status RemoveSection(grpc::ServerContext* context, RemoveSectionRequest const* request, RemoveSectionResponse* response) override;
    grpc::Status MergeSections(grpc::ServerContext* context, MergeSectionsRequest const* request, MergeSectionsResponse* response) override;
    grpc::Status EditSection(grpc::ServerContext* context, EditSectionRequest const* request, EditSectionResponse* response) override;
    grpc::Status MoveSection(grpc::ServerContext* context, MoveSectionRequest const* request, MoveSectionResponse* response) override;
    grpc::Status SearchSections(grpc::ServerContext* context, SearchSectionsRequest const* request, SearchSectionsResponse* response) override;

    // shared functions in section and topic pages
    grpc::Status CreateCard(grpc::ServerContext* context, CreateCardRequest const* request, CreateCardResponse* response) override;
    grpc::Status AddCardToSection(grpc::ServerContext* context, AddCardToSectionRequest const* request, AddCardToSectionResponse* response) override;
    grpc::Status AddCardToTopic(grpc::ServerContext* context, AddCardToTopicRequest const* request, AddCardToTopicResponse* response) override;
    grpc::Status RemoveCard(grpc::ServerContext* context, RemoveCardRequest const* request, RemoveCardResponse* response) override;
    grpc::Status MergeCards(grpc::ServerContext* context, MergeCardsRequest const* request, MergeCardsResponse* response) override;
    grpc::Status SearchCards(grpc::ServerContext* context, SearchCardsRequest const* request, SearchCardsResponse* response) override;

    // section page
    grpc::Status MoveCardToSection(grpc::ServerContext* context, MoveCardToSectionRequest const* request, MoveCardToSectionResponse* response) override;
    grpc::Status MarkSectionAsReviewed(grpc::ServerContext* context, MarkSectionAsReviewedRequest const* request, MarkSectionAsReviewedResponse* response) override;

    // topic page
    grpc::Status GetPracticeCards(grpc::ServerContext* context, GetPracticeCardsRequest const* request, GetPracticeCardsResponse* response) override;
    grpc::Status MoveCardToTopic(grpc::ServerContext* context, MoveCardToTopicRequest const* request, MoveCardToTopicResponse* response) override;
    grpc::Status CreateAssessment(grpc::ServerContext* context, CreateAssessmentRequest const* request, CreateAssessmentResponse* response) override;
    grpc::Status GetAssessments(grpc::ServerContext* context, GetAssessmentsRequest const* request, GetAssessmentsResponse* response) override;
    grpc::Status ExpandAssessment(grpc::ServerContext* context, ExpandAssessmentRequest const* request, ExpandAssessmentResponse* response) override;
    grpc::Status DiminishAssessment(grpc::ServerContext* context, DiminishAssessmentRequest const* request, DiminishAssessmentResponse* response) override;
    grpc::Status IsAssimilated(grpc::ServerContext* context, IsAssimilatedRequest const* request, IsAssimilatedResponse* response) override;

    // card page
    grpc::Status EditCard(grpc::ServerContext* context, EditCardRequest const* request, EditCardResponse* response) override;
    grpc::Status CreateBlock(grpc::ServerContext* context, CreateBlockRequest const* request, CreateBlockResponse* response) override;
    grpc::Status GetBlocks(grpc::ServerContext* context, GetBlocksRequest const* request, GetBlocksResponse* response) override;
    grpc::Status RemoveBlock(grpc::ServerContext* context, RemoveBlockRequest const* request, RemoveBlockResponse* response) override;
    grpc::Status EditBlock(grpc::ServerContext* context, EditBlockRequest const* request, EditBlockResponse* response) override;
    grpc::Status ReorderBlock(grpc::ServerContext* context, ReorderBlockRequest const* request, ReorderBlockResponse* response) override;
    grpc::Status MergeBlocks(grpc::ServerContext* context, MergeBlocksRequest const* request, MergeBlocksResponse* response) override;
    grpc::Status SplitBlock(grpc::ServerContext* context, SplitBlockRequest const* request, SplitBlockResponse* response) override;
    grpc::Status MarkCardAsReviewed(grpc::ServerContext* context, MarkCardAsReviewedRequest const* request, MarkCardAsReviewedResponse* response) override;
    // grpc::Status GetVariations(grpc::ServerContext* context, GetVariationsRequest const* request, GetVariationsResponse* response) override;
    grpc::Status GetSectionCards(grpc::ServerContext* context, GetSectionCardsRequest const* request, GetSectionCardsResponse* response) override;
    grpc::Status GetTopicCards(grpc::ServerContext* context, GetTopicCardsRequest const* request, GetTopicCardsResponse* response) override;

    // progress
    grpc::Status Study(grpc::ServerContext* context, StudyRequest const* request, StudyResponse* response) override;
    grpc::Status MakeProgress(grpc::ServerContext* context, MakeProgressRequest const* request, MakeProgressResponse* response) override;
    grpc::Status GetProgressWeight(grpc::ServerContext* context, GetProgressWeightRequest const* request, GetProgressWeightResponse* response) override;

protected:
    [[nodiscard]] static std::string calculate_hash(std::string_view password);
    [[nodiscard]] static bool password_is_valid(std::string_view lhs, std::string_view rhs);
    [[nodiscard]] static std::string generate_token();
    [[nodiscard]] bool session_is_valid(User const& user) const;

    std::shared_ptr<basic_database> m_database;
};
} // flashback
