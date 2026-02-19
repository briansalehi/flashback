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
                        position: study.getPosition()
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
            block.setExtension(blockExtension);
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
                        extension: block.getExtension(),
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
                        extension: block.getExtension(),
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

    async makeProgress(cardId, duration, practiceMode) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.MakeProgressRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            request.setDuration(duration);
            request.setMode(practiceMode);

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

    async createNerve(subjectId, resourceId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateNerveRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const subject = new proto.flashback.Subject();
            subject.setId(subjectId);
            request.setSubject(subject);
            const resource = new proto.flashback.Resource();
            resource.setId(resourceId);
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

    async editBlock(cardId, blockType, blockExtension, blockMetadata, blockContent) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.EditBlockRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);
            const card = new proto.flashback.Card();
            card.setId(cardId);
            request.setCard(card);
            const block = new proto.flashback.Block();
            block.setType(blockType);
            block.setExtension(blockExtension);
            block.setMetadata(blockMetadata);
            block.setContent(blockContent);
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

    // verifySession { User user = 1; }  { bool valid = 1; }
    // resetPassword { User user = 1; }  { }
    // editUser { User user = 1; }  { }
    // verifyUser { User user = 1; }  { }
    // renameRoadmap { User user = 1; Roadmap roadmap = 2; }  { }
    // removeRoadmap { User user = 1; Roadmap roadmap = 2; }  { }
    // searchRoadmaps { User user = 1; string token = 2; }  { repeated Roadmap roadmap = 1; }
    // cloneRoadmap { User user = 1; Roadmap roadmap = 2; }  { Roadmap roadmap = 1; bool success = 2; string details = 3; uint32 code = 4; }
    // addRequirement { User user = 1; Roadmap roadmap = 2; Milestone milestone = 3; Milestone required_milestone = 4; }  { }
    // getRequirements { User user = 1; Roadmap roadmap = 2; Milestone milestone = 3; }  { repeated Milestone milestones = 1; bool success = 2; string details = 3; uint32 code = 4; }
    // reorderMilestone { User user = 1; Roadmap roadmap = 2; uint64 current_position = 3; uint64 target_position = 4; }  { }
    // removeMilestone { User user = 1; Roadmap roadmap = 2; Milestone milestone = 3; }  { }
    // changeMilestoneLevel { User user = 1; Roadmap roadmap = 2; Milestone milestone = 3; }  { }
    // renameSubject { User user = 1; uint64 id = 2; string name = 3; }  { }
    // removeSubject { User user = 1; Subject subject = 2; }  { }
    // mergeSubjects { User user = 1; Subject source_subject = 2; Subject target_subject = 3; }  { }
    // dropResourceFromSubject { User user = 1; Resource resource = 2; Subject subject = 3; }  { }
    // mergeResources { User user = 1; Resource source = 2; Resource target = 3; }  { }
    // searchResources { User user = 1; string search_token = 2; }  {  repeated ResourceSearchResult results = 4; }
    // reorderTopic { User user = 1; Subject subject = 2; Topic topic = 3; Topic target = 4; }  { }
    // removeTopic { User user = 1; Subject subject = 2; Topic topic = 3; }  { }
    // mergeTopics { User user = 1; Subject subject = 2; Topic source = 3; Topic target = 4; }  { }
    // moveTopic { User user = 1; Subject source_subject = 2; Topic source_topic = 3; Subject target_subject = 4; Topic target_topic = 5; }  { }
    // searchTopics { User user = 1; Subject subject = 2; expertise_level level = 3; string search_token = 4; }  { repeated TopicSearchResult results = 1; }
    // removeResource { User user = 1; Resource resource = 2; }  { }
    // editResource { User user = 1; Resource resource = 2; }  { }
    // reorderSection { User user = 1; Resource resource = 2; Section source = 3; Section target = 4; }  { }
    // removeSection { User user = 1; Resource resource = 2; Section section = 3; }  { }
    // mergeSections { User user = 1; Resource resource = 2; Section source = 3; Section target = 4; }  { }
    // moveSection { User user = 1; Resource source_resource = 2; Section source_section = 3; Resource target_resource = 4; Section target_section = 5;   }  { }
    // createProvider { User user = 1; Provider provider = 2; }  { Provider provider = 1; }
    // searchProviders { User user = 1; string search_token = 2; }  { ProviderSearchResult result = 1; }
    // addProvider { User user = 1; Resource resource = 2; Provider provider = 3; }  { }
    // dropProvider { User user = 1; Resource resource = 2; Provider provider = 3; }  { }
    // renameProvider { User user = 1; Provider provider = 2; }  { }
    // removeProvider { User user = 1; Provider provider = 2; }  { }
    // mergeProviders { User user = 1; Provider source = 2; Provider target = 3; }  { }
    // createPresenter { User user = 1; Presenter presenter = 2; }  { Presenter presenter = 1; }
    // searchPresenters { User user = 1; string search_token = 2; }  { PresenterSearchResult result = 1; }
    // addPresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    // dropPresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    // renamePresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    // removePresenter { User user = 1; Resource resource = 2; Presenter presenter = 3; }  { }
    // mergePresenters { User user = 1; Presenter source = 2; Presenter target = 3; }  { }
    // removeCard { User user = 1; Card card = 2; }  { }
    // mergeCards { User user = 1; Card source = 2; Card target = 3; }  { }
    // editSection { User user = 1; Resource resource = 2; Section section = 3; }  { }
    // moveCardToSection { User user = 1; Card card = 2;; Resource resource = 3; Section source = 4; Section target = 5; }  { }
    // searchSections { User user = 1; Resource resource = 2; string search_token = 3; }  { repeated SectionSearchResult result = 1; }
    // editTopic { User user = 1; Subject subject = 2; Topic topic = 3; Topic target = 4; }  { }
    // moveCardToTopic { User user = 1; Card card = 2; Subject subject = 3; Topic topic = 4; Subject target_subject = 5; Topic target_topic = 6; }  { }
    // searchCards { User user = 1; Subject subject = 2; expertise_level level = 3; string search_token = 4; }  { repeated CardSearchResult result = 1; }
    // removeBlock { User user = 1; Card card = 2; Block block = 3; }  { }
    // reorderBlock { User user = 1; Card card = 2; Block block = 3; Block target = 4; }  { }
    // mergeBlocks { User user = 1; Card card = 2; Block block = 3; Block target = 5; }  { }
    // splitBlock { User user = 1; Card card = 2; Block block = 3; }  { repeated Block block = 1; }
    // moveBlock { User user = 1; Card card = 2; Block block = 3; Card target_card = 4; Block target_block = 5; }  { }
    // createAssessment { User user = 1; Card card = 2; Subject subject = 3; Topic topic = 4; }  { Card card = 1; }
    // getAssessments { User user = 1; Subject subject = 2; Topic topic = 3; }  { repeated Assessment assessment = 1; }
    // expandAssessment { User user = 1; Card card = 2; Subject subject = 3; Topic topic = 4; }  { }
    // diminishAssessment { User user = 1; Card card = 2; Subject subject = 3; Topic topic = 4; }  { }
    // isAssimilated { User user = 1; Subject subject = 2; Topic topic = 3; }  { bool is_assimilated = 1; }
    // getProgressWeight { User user = 1; }  { repeated Weight weight = 1; }
    // estimateCardTime { User user = 1; }  { }
}

const client = new FlashbackClient();
