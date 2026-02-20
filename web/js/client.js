class FlashbackClient {
    constructor() {
        this.apiUrl = window.location.hostname === 'localhost' ? 'http://localhost/api' : 'https://api.flashback.eu.com';
        this.client = null;
        this.ready = false;
        this.device = this.getDevice();
        this.token = this.getToken();

        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.initClient());
        } else {
            this.initClient();
        }
    }

    initClient() {
        try {
            this.client = new window.server_grpc_web_pb.ServerClient(this.apiUrl, null, null);
            this.ready = true;
        } catch (e) {
            console.error('Error initializing client:', e);
        }
    }

    async waitForReady() {
        if (this.ready) return;

        return new Promise((resolve) => {
            const checkReady = () => {
                if (this.ready) {
                    resolve();
                } else {
                    setTimeout(checkReady, 50);
                }
            };
            checkReady();
        });
    }

    getToken() {
        return localStorage.getItem('token') || '';
    }

    getDevice() {
        let device = localStorage.getItem('device');

        if (!device) {
            // UUID v4
            device = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
                const r = Math.random() * 16 | 0;
                const v = c === 'x' ? r : (r & 0x3 | 0x8);
                return v.toString(16);
            });

            localStorage.setItem('device', device);
        }

        return device;
    }

    getMetadata() {
        return {
            'token': `${this.getToken()}`,
            'device': `${this.getDevice()}`
        };
    }

    isAuthenticated() {
        return !!localStorage.getItem('token');
    }

    getAuthenticatedUser() {
        const user = new proto.flashback.User();
        user.setToken(this.getToken());
        user.setDevice(this.getDevice());
        return user;
    }

    handleError(err) {
        if (err.code === 16) { // grpc::StatusCode::UNAUTHENTICATED
            localStorage.removeItem('token');
            this.token = '';
            window.location.href = '/home.html';
        }
        return err;
    }

    getLevel(level) {
        let name;
        switch (level) {
            case 0: name = "Surface"; break;
            case 1: name = "Depth"; break;
            case 2: name = "Origin"; break;
        }
        return name;
    };

    async signOut() {
        await this.waitForReady();

        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SignOutRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.signOut(request, {}, (err) => {
                if (err) {
                    console.error('SignOut error:', err);
                    reject(err);
                } else {
                    localStorage.removeItem('token');
                    this.token = '';
                    resolve();
                }
            });
        });
    }

    async signIn(email, password) {
        await this.waitForReady();

        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SignInRequest();
            const user = new proto.flashback.User();
            user.setEmail(email);
            user.setPassword(password);
            user.setDevice(this.device);
            request.setUser(user);

            this.client.signIn(request, {}, (err, response) => {
                if (err) {
                    console.error('SignIn error:', err);
                    reject(err);
                } else {
                    const user = response.getUser();
                    this.token = user.getToken();
                    localStorage.setItem('token', user.getToken());
                    resolve();
                }
            });
        });
    }

    async signUp(name, email, password) {
        await this.waitForReady();

        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SignUpRequest();
            const user = new proto.flashback.User();
            user.setName(name);
            user.setEmail(email);
            user.setPassword(password);
            user.setDevice(this.getDevice());
            request.setUser(user);

            this.client.signUp(request, {}, (err) => {
                if (err) {
                    console.error('SignUp error:', err);
                    reject(err);
                } else {
                    resolve();
                }
            });
        });
    }

    async getRoadmaps() {
        return new Promise((resolve, reject) => {
            const user = new proto.flashback.User();
            const request = new proto.flashback.GetRoadmapsRequest();
            user.setDevice(this.device);
            user.setToken(this.token);
            request.setUser(user);

            if (!this.isAuthenticated()) {
                console.error('User not logged in:', err);
                reject(err);
                return;
            }

            this.client.getRoadmaps(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('getRoadmaps error:', err);
                    reject(this.handleError(err));
                } else {
                    const roadmaps = response.getRoadmapList().map(roadmap => ({
                        id: roadmap.getId(),
                        name: roadmap.getName()
                    }));
                    resolve(roadmaps);
                }
            });
        });
    }

    async createRoadmap(name) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateRoadmapRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setName(name);

            if (!this.isAuthenticated()) {
                console.error('User is not logged in:', err);
                reject(err);
                return;
            }

            this.client.createRoadmap(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('createRoadmap error:', err);
                    reject(this.handleError(err));
                } else {
                    const roadmap = response.getRoadmap();
                    resolve({
                        id: roadmap.getId(),
                        name: roadmap.getName(),
                    });
                }
            });
        });
    }

    async getMilestones(roadmapId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetMilestonesRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setRoadmapId(roadmapId);

            this.client.getMilestones(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('GetRoadmap error:', err);
                    reject(this.handleError(err));
                } else {
                    const milestones = response.getMilestonesList().map(ms => ({
                        id: ms.getId(),
                        position: ms.getPosition(),
                        name: ms.getName(),
                        level: ms.getLevel(),
                    }));

                    resolve({
                        milestones: milestones
                    });
                }
            });
        });
    }

    async searchSubjects(searchToken) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SearchSubjectsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setToken(searchToken);

            this.client.searchSubjects(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('SearchSubjects error:', err);
                    reject(this.handleError(err));
                } else {
                    const subjects = response.getSubjectsList().map(match => ({
                        id: match.getSubject().getId(),
                        name: match.getSubject().getName()
                    }));
                    resolve(subjects);
                }
            });
        });
    }

    async createSubject(name) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateSubjectRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setName(name);

            this.client.createSubject(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('CreateSubject error:', err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async addMilestone(roadmapId, subjectId, level) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.AddMilestoneRequest();
            const user = this.getAuthenticatedUser();

            request.setUser(user);
            request.setSubjectId(subjectId);
            request.setSubjectLevel(level);
            request.setRoadmapId(roadmapId);
            request.setPosition(0);

            this.client.addMilestone(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('AddMilestone error:', err);
                    reject(this.handleError(err));
                } else {
                    const milestone = response.getMilestone();
                    resolve({
                        milestone: milestone ? {
                            id: milestone.getId(),
                            name: milestone.getName(),
                            level: this.getLevel(milestone.getLevel()),
                            position: milestone.getPosition()
                        } : null
                    });
                }
            });
        });
    }

    async getResources(subjectId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetResourcesRequest();
            const user = this.getAuthenticatedUser();
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            request.setUser(user);

            this.client.getResources(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('GetResources error:', err);
                    reject(this.handleError(err));
                } else {
                    const resources = response.getResourcesList().map(res => ({
                        id: res.getId(),
                        type: res.getType(),
                        pattern: res.getPattern(),
                        name: res.getName(),
                        link: res.getLink(),
                        production: res.getProduction(),
                        expiration: res.getExpiration()
                    }));
                    resolve(resources);
                }
            });
        });
    }

    async createResource(name, type, pattern, link, production, expiration) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateResourceRequest();
            const user = this.getAuthenticatedUser();
            const resource = new proto.flashback.Resource();
            resource.setName(name);
            resource.setType(type);
            resource.setPattern(pattern);
            resource.setLink(link);
            resource.setProduction(production);
            resource.setExpiration(expiration);
            request.setResource(resource);
            request.setUser(user);

            this.client.createResource(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('CreateResource:', err);
                    reject(this.handleError(err));
                } else {
                    const resource = response.getResource();
                    resolve({
                        id: resource.getId(),
                        type: resource.getType(),
                        pattern: resource.getPattern(),
                        name: resource.getName(),
                        link: resource.getLink(),
                        production: resource.getProduction(),
                        expiration: resource.getExpiration()
                    });
                }
            });
        });
    }

    async addResourceToSubject(subjectId, resourceId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.AddResourceToSubjectRequest();
            const user = this.getAuthenticatedUser();
            const subject = new proto.flashback.Subject();
            const resource = new proto.flashback.Resource();
            subject.setId(subjectId);
            resource.setId(resourceId);
            request.setSubject(subject);
            request.setResource(resource);
            request.setUser(user);

            this.client.addResourceToSubject(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('AddResource error:', err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    // position can be set to 0 so that topic takes the last position in subject
    async createTopic(subjectId, topicName, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateTopicRequest();
            const user = this.getAuthenticatedUser();
            const subject = new proto.flashback.Subject();
            const topic = new proto.flashback.Topic();
            subject.setId(subjectId);
            topic.setName(topicName);
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setSubject(subject);
            request.setTopic(topic);
            request.setUser(user);

            this.client.createTopic(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('CreateTopic error:', err);
                    reject(this.handleError(err));
                } else {
                    const topic = response.getTopic();
                    resolve({
                        name: topic.getName(),
                        level: topic.getLevel(),
                        position: topic.getPosition()
                    });
                }
            });
        });
    }

    async getTopics(subjectId, level) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetTopicsRequest();
            const user = this.getAuthenticatedUser();
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            request.setLevel(level);
            request.setUser(user);

            this.client.getTopics(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetTopics error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getTopicList().map(topic => ({
                        name: topic.getName(),
                        level: topic.getLevel(),
                        position: topic.getPosition()
                    })));
                }
            });
        });
    }

    // section position can be 0 so that it takes the last position automatically
    async createSection(resourceId, sectionName, sectionLink, sectionPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateSectionRequest();
            const user = this.getAuthenticatedUser();
            const resource = new proto.flashback.Resource();
            const section = new proto.flashback.Section();
            resource.setId(resourceId);
            section.setName(sectionName);
            section.setLink(sectionLink);
            section.setPosition(sectionPosition);
            request.setResource(resource);
            request.setSection(section);
            request.setUser(user);

            this.client.createSection(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("CreateSection error:", err);
                    reject(this.handleError(err));
                } else {
                    const section = response.getSection();
                    resolve({
                        name: section.getName(),
                        link: section.getLink(),
                        position: section.getPosition()
                    });
                }
            });
        });
    }

    async getSections(resourceId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetSectionsRequest();
            const user = this.getAuthenticatedUser();
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);
            request.setUser(user);

            this.client.getSections(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetSections error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getSectionList().map(section => ({
                        name: section.getName(),
                        link: section.getLink(),
                        position: section.getPosition(),
                        state: section.getState()
                    })));
                }
            });
        });
    }

    async getStudyResources() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetStudyResourcesRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.getStudyResources(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetStudyResources error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getStudyList().map(study => ({
                        id: study.getResource().getId(),
                        name: study.getResource().getName(),
                        type: study.getResource().getType(),
                        pattern: study.getResource().getPattern(),
                        production: study.getResource().getProduction(),
                        expiration: study.getResource().getExpiration(),
                        link: study.getResource().getLink(),
                        order: study.getOrder()
                    })));
                }
            });
        });
    }

    async createCard(headline) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateCardRequest();
            const user = this.getAuthenticatedUser();
            const card = new proto.flashback.Card();
            card.setHeadline(headline);
            request.setCard(card);
            request.setUser(user);

            this.client.createCard(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("CreateCard error:", err);
                    reject(this.handleError(err));
                } else {
                    const card = response.getCard();
                    resolve({
                        id: card.getId(),
                        headline: card.getHeadline(),
                        state: card.getState()
                    });
                }
            });
        });
    }

    async addCardToSection(cardId, resourceId, sectionPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.AddCardToSectionRequest();
            const user = this.getAuthenticatedUser();
            const card = new proto.flashback.Card();
            const resource = new proto.flashback.Resource();
            const section = new proto.flashback.Section();
            resource.setId(resourceId);
            card.setId(cardId);
            section.setPosition(sectionPosition);
            request.setResource(resource);
            request.setSection(section);
            request.setUser(user);
            request.setCard(card);

            this.client.addCardToSection(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("AddCardToSection error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async addCardToTopic(cardId, subjectId, topicPosition, topicLevel) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.AddCardToTopicRequest();
            const user = this.getAuthenticatedUser();
            const card = new proto.flashback.Card();
            const subject = new proto.flashback.Subject();
            const topic = new proto.flashback.Topic();
            card.setId(cardId);
            subject.setId(subjectId);
            topic.setPosition(topicPosition);
            topic.setLevel(topicLevel);
            request.setCard(card);
            request.setSubject(subject);
            request.setTopic(topic);
            request.setUser(user);

            this.client.addCardToTopic(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("AddCardToTopic error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async getSectionCards(resourceId, sectionPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetSectionCardsRequest();
            const user = this.getAuthenticatedUser();
            const resource = new proto.flashback.Resource();
            const section = new proto.flashback.Section();
            resource.setId(resourceId);
            section.setPosition(sectionPosition);
            request.setResource(resource);
            request.setSection(section);
            request.setUser(user);

            this.client.getSectionCards(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetSectionCards error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getCardList().map(card => ({
                        id: card.getId(),
                        headline: card.getHeadline(),
                        state: card.getState()
                    })));
                }
            });
        });
    }

    async getTopicCards(subjectId, topicPosition, topicLevel) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetTopicCardsRequest();
            const user = this.getAuthenticatedUser();
            const subject = new proto.flashback.Subject();
            const topic = new proto.flashback.Topic();
            subject.setId(subjectId);
            topic.setPosition(topicPosition);
            topic.setLevel(topicLevel);
            request.setUser(user);
            request.setSubject(subject);
            request.setTopic(topic);

            this.client.getTopicCards(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetTopicCards error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getCardList().map(card => ({
                        id: card.getId(),
                        headline: card.getHeadline(),
                        state: card.getState()
                    })));
                }
            });
        });
    }

    // position can be set to 0 so that block takes the last position
    async createBlock(cardId, blockType, blockExtension, blockContent, blockMetadata) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateBlockRequest();
            const user = this.getAuthenticatedUser();
            const card = new proto.flashback.Card();
            const block = new proto.flashback.Block();
            card.setId(cardId);
            block.setType(blockType);
            block.setMetadata(blockMetadata);
            block.setExtension$(blockExtension);
            block.setContent(blockContent);
            request.setCard(card);
            request.setBlock(block);
            request.setUser(user);

            this.client.createBlock(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("CreateBlock error:", err);
                    reject(this.handleError(err));
                } else {
                    const block = response.getBlock();
                    resolve({
                        position: block.getPosition(),
                        type: block.getType(),
                        extension: block.getExtension$(),
                        content: block.getContent(),
                        metadata: block.getMetadata()
                    });
                }
            });
        });
    }

    async getBlocks(cardId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetBlocksRequest();
            const user = this.getAuthenticatedUser();
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            request.setUser(user);

            this.client.getBlocks(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetBlocks error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getBlockList().map(block => ({
                        position: block.getPosition(),
                        type: block.getType(),
                        extension: block.getExtension$(),
                        content: block.getContent(),
                        metadata: block.getMetadata()
                    })));
                }
            });
        });
    }

    async getPracticeTopics(roadmapId, subjectId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetPracticeTopicsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            request.setRoadmap(roadmap);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);

            this.client.getPracticeTopics(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetPracticeTopics error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getTopicList().map(topic => ({
                        position: topic.getPosition(),
                        name: topic.getName(),
                        level: topic.getLevel()
                    })));
                }
            });
        });
    }

    async getPracticeCards(roadmapId, subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetPracticeCardsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            request.setRoadmap(roadmap);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.getPracticeCards(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetPracticeCards error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getCardList().map(card => ({
                        id: card.getId(),
                        headline: card.getHeadline(),
                        state: card.getState()
                    })));
                }
            });
        });
    }

    async makeProgress(cardId, duration, milestoneId, milestoneLevel) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MakeProgressRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const milestone = new proto.flashback.Milestone();
            milestone.setId(milestoneId);
            milestone.setLevel(milestoneLevel);
            request.setMilestone(milestone);
            request.setDuration(duration);

            this.client.makeProgress(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MakeProgress error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async markCardAsReviewed(cardId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MarkCardAsReviewedRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);

            this.client.markCardAsReviewed(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("MarkCardAsReviewed error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async markSectionAsReviewed(resourceId, sectionPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MarkSectionAsReviewedRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);
            const section = new proto.flashback.Section();
            section.setPosition(sectionPosition);
            request.setSection(section);

            this.client.markSectionAsReviewed(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MarkSectionAsReviewed error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async study(cardId, duration) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.StudyRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            request.setDuration(duration);

            this.client.study(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("Study error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async createNerve(subjectId, resourceName, resourceExpiration) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateNerveRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const resource = new proto.flashback.Resource();
            resource.setName(resourceName);
            resource.setExpiration(resourceExpiration);
            request.setResource(resource);

            this.client.createNerve(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("CreateNerve error:", err);
                    reject(this.handleError(err));
                } else {
                    const resource = response.getResource();
                    resolve({
                        id: resource.getId(),
                        name: resource.getName(),
                        type: resource.getType(),
                        pattern: resource.getPattern(),
                        production: resource.getProduction(),
                        expiration: resource.getExpiration(),
                        link: resource.getLink()
                    });
                }
            });
        });
    }

    async getNerves() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetNervesRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.getNerves(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetNerves error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getResourcesList().map(resource => ({
                        id: resource.getId(),
                        name: resource.getName(),
                        type: resource.getType(),
                        pattern: resource.getPattern(),
                        production: resource.getProduction(),
                        expiration: resource.getExpiration(),
                        link: resource.getLink()
                    })));
                }
            });
        });
    }

    async editCard(cardId, cardHeadline) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditCardRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            card.setHeadline(cardHeadline);
            request.setCard(card);

            this.client.editCard(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("EditCard error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async editBlock(cardId, blockPosition, blockType, blockExtension, blockContent, blockMetadata) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditBlockRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const block = new proto.flashback.Block();
            block.setPosition(blockPosition);
            block.setType(blockType);
            block.setExtension$(blockExtension);
            block.setContent(blockContent);
            block.setMetadata(blockMetadata);
            request.setBlock(block);

            this.client.editBlock(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("EditBlock error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async resetPassword() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ResetPasswordRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.resetPassword(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ResetPassword error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async editUser(name, email) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditUserRequest();
            const user = this.getAuthenticatedUser();
            user.setName(name);
            user.setEmail(email);
            request.setUser(user);

            this.client.editUser(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("EditUser error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async renameRoadmap(roadmapId, roadmapName) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RenameRoadmapRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            roadmap.setName(roadmapName);
            request.setRoadmap(roadmap);

            this.client.renameRoadmap(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RenameRoadmap error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async removeRoadmap(roadmapId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveRoadmapRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            request.setRoadmap(roadmap);

            this.client.removeRoadmap(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveRoadmap error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async searchRoadmaps(token) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SearchRoadmapsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setToken(token);

            this.client.searchRoadmaps(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("SearchRoadmaps error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getRoadmapList().map(roadmap => ({
                        id: roadmap.getId(),
                        name: roadmap.getName()
                    })));
                }
            });
        });
    }

    async cloneRoadmap(roadmapId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CloneRoadmapRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            request.setRoadmap(roadmap);

            this.client.cloneRoadmap(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("CloneRoadmap error:", err);
                    reject(this.handleError(err));
                } else {
                    const roadmap = response.getRoadmap();
                    resolve({
                        id: roadmap.getId(),
                        name: roadmap.getName()
                    });
                }
            });
        });
    }

    async reorderMilestone(roadmapId, currentPosition, targetPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ReorderMilestoneRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            request.setRoadmap(roadmap);
            request.setCurrentPosition(currentPosition);
            request.setTargetPosition(targetPosition);

            this.client.reorderMilestone(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ReorderMilestone error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async removeMilestone(roadmapId, milestoneId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveMilestoneRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            const milestone = new proto.flashback.Milestone();
            milestone.setId(milestoneId);
            request.setRoadmap(roadmap);
            request.setMilestone(milestone);

            this.client.removeMilestone(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveMilestone error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async changeMilestoneLevel(roadmapId, milestoneId, milestoneLevel) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ChangeMilestoneLevelRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const roadmap = new proto.flashback.Roadmap();
            roadmap.setId(roadmapId);
            const milestone = new proto.flashback.Milestone();
            milestone.setId(milestoneId);
            milestone.setLevel(milestoneLevel);
            request.setRoadmap(roadmap);
            request.setMilestone(milestone);

            this.client.changeMilestoneLevel(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ChangeMilestoneLevel error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async renameSubject(subjectId, subjectName) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RenameSubjectRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setId(subjectId);
            request.setName(subjectName);

            this.client.renameSubject(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RenameSubject error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async removeSubject(subjectId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveSubjectRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);

            this.client.removeSubject(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveSubject error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async dropResourceFromSubject(resourceId, subjectId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.DropResourceFromSubjectRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setResource(resource);
            request.setSubject(subject);

            this.client.dropResourceFromSubject(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("DropResourceFromSubject error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async searchResources(token) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SearchResourcesRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            request.setSearchToken(token);

            this.client.searchResources(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("SearchResources error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getResultsList().map(result => ({
                        id: result.getResource().getId(),
                        name: result.getResource().getName(),
                        type: result.getResource().getType(),
                        pattern: result.getResource().getPattern(),
                        production: result.getResource().getProduction(),
                        expiration: result.getResource().getExpiration(),
                        link: result.getResource().getLink(),
                        position: result.getPosition()
                    })));
                }
            });
        });
    }

    async reorderTopic(subjectId, topicLevel, sourcePosition, targetPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ReorderTopicRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setPosition(sourcePosition);
            topic.setLevel(topicLevel);
            request.setTopic(topic);
            const target = new proto.flashback.Topic();
            target.setPosition(targetPosition);
            target.setLevel(topicLevel);
            request.setTarget(target);

            this.client.reorderTopic(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ReorderTopic error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async removeTopic(subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveTopicRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.removeTopic(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveTopic error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async moveTopic(subjectId, sourceLevel, sourcePosition, targetLevel, targetPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MoveTopicRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            const source = new proto.flashback.Topic();
            source.setLevel(sourceLevel);
            source.setPosition(sourcePosition);
            request.setSource(source);
            const target = new proto.flashback.Topic();
            target.setLevel(targetLevel);
            target.setPosition(targetPosition);
            request.setTarget(target);

            this.client.moveTopic(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MoveTopic error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async searchTopics(subjectId, topicLevel, token) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SearchTopicsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            request.setLevel(topicLevel);
            request.setSearchToken(token);

            this.client.searchTopics(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("SearchTopics error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getResultsList().map(result => ({
                        position: result.getTopic().getPosition(),
                        level: result.getTopic().getLevel(),
                        name: result.getTopic().getName(),
                        order: result.getPosition()
                    })));
                }
            });
        });
    }

    async removeResource(resourceId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveResourceRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);

            this.client.removeResource(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveResource error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async editResource(resourceId, resourceType, resourcePattern, resourceName, resourceProduction, resourceExpiration, resourceLink) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditResourceRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            resource.setType(resourceType);
            resource.setPattern(resourcePattern);
            resource.setName(resourceName);
            resource.setProduction(resourceProduction);
            resource.setExpiration(resourceExpiration);
            resource.setLink(resourceLink);
            request.setResource(resource);

            this.client.editResource(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("EditResource error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async reorderSection(resourceId, sourcePosition, targetPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ReorderSectionRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);
            const section = new proto.flashback.Section();
            section.setPosition(sourcePosition);
            request.setSource(section);
            const target = new proto.flashback.Section();
            target.setPosition(targetPosition);
            request.setTarget(target);

            this.client.reorderSection(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ReorderSection error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async removeSection(resourceId, sectionPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveSectionRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);
            const section = new proto.flashback.Section();
            section.setPosition(sectionPosition);
            request.setSection(section);

            this.client.removeSection(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveSection error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async moveSection(sourceResourceId, sourceSectionPosition, targetResourceId, targetSectionPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MoveSectionRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const sourceResource = new proto.flashback.Resource();
            sourceResource.setId(sourceResourceId);
            request.setSourceResource(sourceResource);
            const sourceSection = new proto.flashback.Section();
            sourceSection.setPosition(sourceSectionPosition);
            request.setSourceSection(sourceSection);
            const targetResource = new proto.flashback.Resource();
            targetResource.setId(targetResourceId);
            request.setTargetResource(targetResource);
            const targetSection = new proto.flashback.Section();
            targetSection.setPosition(targetSectionPosition);
            request.setTargetSection(targetSection);

            this.client.moveSection(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MoveSection error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async removeCard(cardId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveCardRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);

            this.client.removeCard(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveCard error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async editSection(resourceId, sectionPosition, sectionName, sectionLink) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditSectionRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);
            const section = new proto.flashback.Section();
            section.setPosition(sectionPosition);
            section.setName(sectionName);
            section.setLink(sectionLink);
            request.setSection(section);

            this.client.editSection(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("EditSection error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async moveCardToSection(cardId, resourceId, sourcePosition, targetPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MoveCardToSectionRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            const source = new proto.flashback.Section();
            source.setPosition(sourcePosition);
            const target = new proto.flashback.Section();
            target.setPosition(targetPosition);
            request.setCard(card);
            request.setResource(resource);
            request.setSource(source);
            request.setTarget(target);

            this.client.moveCardToSection(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MoveCardToSection error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async searchSections(resourceId, token) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SearchSectionsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
            request.setResource(resource);
            request.setSearchToken(token);

            this.client.searchSections(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("SearchSections error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getResultList().map(result => ({
                        position: result.getSection().getPosition(),
                        name: result.getSection().getName(),
                        link: result.getSection().getLink(),
                        order: result.getPosition()
                    })));
                }
            });
        });
    }

    async editTopic(subjectId, topicLevel, topicPosition, topicName, targetLevel) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditTopicRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            topic.setName(topicName);
            request.setTopic(topic);
            const target = new proto.flashback.Topic();
            target.setLevel(targetLevel);
            target.setPosition(topicPosition);
            target.setName(topicName);
            request.setTarget(target);


            this.client.editTopic(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("EditTopic error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async moveCardToTopic(cardId, subjectId, topicLevel, topicPosition, targetSubjectId, targetTopicLevel, targetTopicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MoveCardToTopicRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setPosition(topicPosition);
            request.setTopic(topic);
            const targetSubject = new proto.flashback.Subject();
            targetSubject.setId(targetSubjectId);
            request.setTargetSubject(targetSubject);
            const targetTopic = new proto.flashback.Topic();
            targetTopic.setPosition(targetTopicPosition);
            targetTopic.setLevel(targetTopicLevel);
            request.setTargetTopic(targetTopic);

            this.client.moveCardToTopic(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MoveCardToTopic error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async searchCards(subjectId, level, token) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SearchCardsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            request.setLevel(level);
            request.setSearchToken(token);

            this.client.searchCards(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("SearchCards error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getResultList().map(result => ({
                        id: result.getCard().getId(),
                        headline: result.getCard().getHeadline(),
                        state: result.getCard().getState(),
                        order: result.position()
                    })));
                }
            });
        });
    }

    async removeBlock(cardId, blockPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.RemoveBlockRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const block = new proto.flashback.Block();
            block.setPosition(blockPosition);
            request.setBlock(block);

            this.client.removeBlock(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("RemoveBlock error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    // reorderBlock { User user = 1; Card card = 2; Block block = 3; Block target = 4; }  { }
    async reorderBlock(cardId, blockPosition, targetPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ReorderBlockRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const block = new proto.flashback.Block();
            block.setPosition(blockPosition);
            request.setBlock(block);
            const target = new proto.flashback.Block();
            target.setPosition(targetPosition);
            request.setTarget(target);

            this.client.reorderBlock(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ReorderBlock error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async splitBlock(cardId, blockPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SplitBlockRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const block = new proto.flashback.Block();
            block.setPosition(blockPosition);
            request.setBlock(block);

            this.client.splitBlock(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("SplitBlock error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getBlockList().map(block => ({
                        position: block.getPosition(),
                        type: block.getType(),
                        extension: block.getExtension$(),
                        metadata: block.getMetadata(),
                        content: block.getContent()
                    })));
                }
            });
        });
    }

    async moveBlock(cardId, blockPosition, targetCardId, targetBlockPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MoveBlockRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const block = new proto.flashback.Block();
            block.setPosition(blockPosition);
            request.setBlock(block);
            const targetCard = new proto.flashback.Card();
            targetCard.setId(targetCardId);
            request.setTargetCard(targetCard);
            const targetBlock = new proto.flashback.Block();
            targetBlock.setPosition(targetBlockPosition);
            request.setTargetBlock(targetBlock);

            this.client.moveBlock(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("MoveBlock error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async createAssessment(cardId, subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateAssessmentRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.createAssessment(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("CreateAssessment error:", err);
                    reject(this.handleError(err));
                } else {
                    const card = response.getCard();
                    resolve({
                        id: card.id(),
                        headline: card.getHeadline(),
                        state: card.getState()
                    });
                }
            });
        });
    }

    async getAssessments(subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetAssessmentsRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.getAssessments(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("GetAssessments error:", err);
                    reject(this.handleError(err));
                } else {
                    const assessment = response.getAssessment();
                    resolve({
                        id: assessment.getId(),
                        headline: assessment.getHeadline(),
                        state: assessment.getState(),
                        assimilations: assessment.getAssimilations()
                    });
                }
            });
        });
    }

    async expandAssessment(cardId, subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.ExpandAssessmentRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.expandAssessment(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("ExpandAssessment error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async diminishAssessment(cardId, subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.DiminishAssessmentRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.diminishAssessment(request, this.getMetadata(), (err) => {
                if (err) {
                    console.error("Diminish Assessment error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve();
                }
            });
        });
    }

    async isAssimilated(subjectId, topicLevel, topicPosition) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.IsAssimilatedRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const topic = new proto.flashback.Topic();
            topic.setLevel(topicLevel);
            topic.setPosition(topicPosition);
            request.setTopic(topic);

            this.client.isAssimilated(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("IsAssimilated error:", err);
                    reject(this.handleError(err));
                } else {
                    resolve(response.getIsAssimilated());
                }
            });
        });
    }

    /*
    // getProgressWeight { User user = 1; }  { repeated Weight weight = 1; }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // createProvider { User user = 1; Provider provider = 2; }  { Provider provider = 1; }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // searchProviders { User user = 1; string search_token = 2; }  { ProviderSearchResult result = 1; }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // addProvider { User user = 1; Resource resource = 2; Provider provider = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // dropProvider { User user = 1; Resource resource = 2; Provider provider = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // renameProvider { User user = 1; Provider provider = 2; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // removeProvider { User user = 1; Provider provider = 2; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // createPresenter { User user = 1; Presenter presenter = 2; }  { Presenter presenter = 1; }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // searchPresenters { User user = 1; string search_token = 2; }  { PresenterSearchResult result = 1; }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // addPresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // dropPresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // renamePresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // removePresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeProviders { User user = 1; Provider source = 2; Provider target = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergePresenters { User user = 1; Presenter source = 2; Presenter target = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeSubjects { User user = 1; Subject source_subject = 2; Subject target_subject = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeResources { User user = 1; Resource source = 2; Resource target = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeTopics { User user = 1; Subject subject = 2; Topic source = 3; Topic target = 4; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeSections { User user = 1; Resource resource = 2; Section source = 3; Section target = 4; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeCards { User user = 1; Card source = 2; Card target = 3; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // mergeBlocks { User user = 1; Card card = 2; Block block = 3; Block target = 5; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // addRequirement { User user = 1; Roadmap roadmap = 2; Milestone milestone = 3; Milestone required_milestone = 4; }  { }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // getRequirements { User user = 1; Roadmap roadmap = 2; Milestone milestone = 3; }  { repeated Milestone milestones = 1; bool success = 2; string details = 3; uint32 code = 4; }
    async xxx() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.xxxRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.xxx(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error("xxx error:", err);
                    reject(this.handleError(err));
                } else {
                    const xxx = response.xxx();
                    resolve(response.getXXXList().map(x => ({
                    })));
                    resolve({
                    });
                    resolve();
                }
            });
        });
    }

    // estimateCardTime { User user = 1; }  { }
    // verifySession { User user = 1; }  { bool valid = 1; }
    // verifyUser { User user = 1; }  { }
    */
}

const client = new FlashbackClient();
