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

    getLevel(level) {
        let name;
        switch (level) {
            case 0: name = "Surface"; break;
            case 1: name = "Depth"; break;
            case 2: name = "Origin"; break;
        }
        return name;
    };

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

    async signOut() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SignOutRequest();
            const user = this.getAuthenticatedUser();
            request.setUser(user);

            this.client.signOut(request, this.getMetadata(), (err) => {
                localStorage.removeItem('token');
                if (err) {
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
                    reject(err);
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
                    reject(err);
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
                    reject(err);
                } else {
                    const milestones = response.getMilestonesList().map(ms => ({
                        id: ms.getId(),
                        position: ms.getPosition(),
                        name: ms.getName(),
                        level: this.getLevel(ms.getLevel()),
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
                    reject(err);
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
                    reject(err);
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
                    reject(err);
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
            request.setSubjectid(subjectId);

            this.client.getResources(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('GetResources error:', err);
                    reject(err);
                } else {
                    const resources = response.getResourcesList().map(res => ({
                        id: res.getId(),
                        name: res.getName(),
                        type: res.getType(),
                        url: res.getUrl()
                    }));
                    resolve(resources);
                }
            });
        });
    }

    async addResource(subjectId, name, type, url) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.AddResourceRequest();
            request.setSubjectid(subjectId);
            request.setName(name);
            request.setType(type);
            request.setUrl(url);

            this.client.addResource(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('AddResource error:', err);
                    reject(err);
                } else {
                    resolve({
                        id: response.getId(),
                        name: response.getName(),
                        type: response.getType(),
                        url: response.getUrl()
                    });
                }
            });
        });
    }
}

const client = new FlashbackClient();
