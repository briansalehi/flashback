class FlashbackClient {
    constructor() {
        this.apiUrl = window.location.hostname === 'localhost' ? 'http://localhost:8080' : 'https://api.flashback.eu.com';
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
    
    getMetadata() {
        const token = this.getToken();
        const device = this.getDevice();
        return {
            'token': `${token}`,
            'device': `${device}`
        };
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

    async signIn(email, password) {
        await this.waitForReady();

        return new Promise((resolve, reject) => {
            const user = new proto.flashback.User();
            const request = new proto.flashback.SignInRequest();
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
                    localStorage.setItem('token', this.token);

                    resolve({
                        token: user.getToken(),
                        device: user.getDevice()
                    });
                }
            });
        });
    }
    
    async signUp(email, password, displayName) {
        await this.waitForReady();
        
        return new Promise((resolve, reject) => {
            const user = new proto.flashback.User();
            const request = new proto.flashback.SignUpRequest();
            user.setName(name);
            user.setEmail(email);
            user.setPassword(password);
            user.setDevice(this.getDevice());
            request.setUser(user);

            this.client.signUp(request, {}, (err, response) => {
                if (err) {
                    console.error('SignUp error:', err);
                    reject(err);
                } else {
                    console.error('SignUp successful');
                    resolve();
                }
            });
        });
    }
    
    async signOut() {
        return new Promise((resolve, reject) => {
            const user = new proto.flashback.User();
            const request = new proto.flashback.SignOutRequest();
            user.setDevice(this.device);
            user.setToken(this.token);
            request.setUser(user);

            this.client.signOut(request, this.getMetadata(), (err, response) => {
                localStorage.removeItem('token');
                if (err) {
                    console.error('SignOut error:', err);
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
    
    async createRoadmap(name, description) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateRoadmapRequest();
            request.setName(name);
            request.setDescription(description);
            
            this.client.createRoadmap(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('CreateRoadmap error:', err);
                    reject(err);
                } else {
                    resolve({
                        id: response.getId(),
                        name: response.getName(),
                        description: response.getDescription()
                    });
                }
            });
        });
    }
    
    async getRoadmap(roadmapId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetRoadmapRequest();
            request.setId(roadmapId);
            
            this.client.getRoadmap(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('GetRoadmap error:', err);
                    reject(err);
                } else {
                    const milestones = response.getMilestonesList().map(ms => ({
                        id: ms.getId(),
                        name: ms.getName(),
                        description: ms.getDescription(),
                        targetDate: ms.getTargetdate() ? new Date(ms.getTargetdate() * 1000) : null,
                        completed: ms.getCompleted()
                    }));
                    
                    resolve({
                        id: response.getId(),
                        name: response.getName(),
                        description: response.getDescription(),
                        milestones: milestones
                    });
                }
            });
        });
    }
    
    async createMilestone(roadmapId, name, description, targetDate) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateMilestoneRequest();
            request.setRoadmapid(roadmapId);
            request.setName(name);
            request.setDescription(description);
            if (targetDate) {
                request.setTargetdate(Math.floor(targetDate.getTime() / 1000));
            }
            
            this.client.createMilestone(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('CreateMilestone error:', err);
                    reject(err);
                } else {
                    resolve({
                        id: response.getId(),
                        name: response.getName(),
                        description: response.getDescription()
                    });
                }
            });
        });
    }
    
    async getSubjects(milestoneId) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetSubjectsRequest();
            request.setMilestoneid(milestoneId);
            
            this.client.getSubjects(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('GetSubjects error:', err);
                    reject(err);
                } else {
                    const subjects = response.getSubjectsList().map(subj => ({
                        id: subj.getId(),
                        name: subj.getName(),
                        description: subj.getDescription()
                    }));
                    resolve(subjects);
                }
            });
        });
    }
    
    async createSubject(milestoneId, name, description) {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.CreateSubjectRequest();
            request.setMilestoneid(milestoneId);
            request.setName(name);
            request.setDescription(description);
            
            this.client.createSubject(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('CreateSubject error:', err);
                    reject(err);
                } else {
                    resolve({
                        id: response.getId(),
                        name: response.getName(),
                        description: response.getDescription()
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

const flashbackClient = new FlashbackClient();
