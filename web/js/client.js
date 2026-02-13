class FlashbackClient {
    constructor() {
        this.apiUrl = 'https://api.flashback.eu.com';
        this.client = null;
        this.ready = false;
        
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
            console.log('gRPC client initialized');
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
    
    /**
     * Get authentication token from session
     */
    getAuthToken() {
        return sessionStorage.getItem('authToken') || '';
    }
    
    /**
     * Get metadata with auth token
     */
    getMetadata() {
        const token = this.getAuthToken();
        return {
            'authorization': `Bearer ${token}`
        };
    }
    
    /**
     * Sign in user
     * @param {string} email 
     * @param {string} password 
     * @returns {Promise<Object>}
     */
    async signIn(email, password) {
        await this.waitForReady();
        
        return new Promise((resolve, reject) => {
            const user = new proto.flashback.User();
            const request = new proto.flashback.SignInRequest();
            user.setEmail(email);
            user.setPassword(password);
            request.setUser(user);
            
            this.client.signIn(request, {}, (err, response) => {
                if (err) {
                    console.error('SignIn error:', err);
                    reject(err);
                } else {
                    // Store auth token
                    const token = response.getToken();
                    sessionStorage.setItem('authToken', token);
                    
                    resolve({
                        token: token,
                        userId: response.getUserid()
                    });
                }
            });
        });
    }
    
    /**
     * Sign up new user
     * @param {string} email 
     * @param {string} password 
     * @param {string} displayName 
     * @returns {Promise<Object>}
     */
    async signUp(email, password, displayName) {
        await this.waitForReady();
        
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SignUpRequest();
            request.setEmail(email);
            request.setPassword(password);
            request.setDisplayname(displayName);
            
            this.client.signUp(request, {}, (err, response) => {
                if (err) {
                    console.error('SignUp error:', err);
                    reject(err);
                } else {
                    // Store auth token
                    const token = response.getToken();
                    sessionStorage.setItem('authToken', token);
                    
                    resolve({
                        token: token,
                        userId: response.getUserid()
                    });
                }
            });
        });
    }
    
    /**
     * Sign out user
     */
    async signOut() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.SignOutRequest();
            
            this.client.signOut(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('SignOut error:', err);
                    // Clear token anyway
                    sessionStorage.removeItem('authToken');
                    reject(err);
                } else {
                    sessionStorage.removeItem('authToken');
                    resolve();
                }
            });
        });
    }
    
    /**
     * Get all roadmaps for current user
     * @returns {Promise<Array>}
     */
    async getRoadmaps() {
        return new Promise((resolve, reject) => {
            const request = new proto.flashback.GetRoadmapsRequest();
            
            this.client.getRoadmaps(request, this.getMetadata(), (err, response) => {
                if (err) {
                    console.error('GetRoadmaps error:', err);
                    reject(err);
                } else {
                    const roadmaps = response.getRoadmapsList().map(rm => ({
                        id: rm.getId(),
                        name: rm.getName(),
                        description: rm.getDescription(),
                        createdAt: new Date(rm.getCreatedat() * 1000)
                    }));
                    resolve(roadmaps);
                }
            });
        });
    }
    
    /**
     * Create a new roadmap
     * @param {string} name 
     * @param {string} description 
     * @returns {Promise<Object>}
     */
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
    
    /**
     * Get roadmap details with milestones
     * @param {string} roadmapId 
     * @returns {Promise<Object>}
     */
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
    
    /**
     * Create a milestone in a roadmap
     * @param {string} roadmapId 
     * @param {string} name 
     * @param {string} description 
     * @param {Date} targetDate 
     * @returns {Promise<Object>}
     */
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
    
    /**
     * Get subjects for a milestone
     * @param {string} milestoneId 
     * @returns {Promise<Array>}
     */
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
    
    /**
     * Create a subject in a milestone
     * @param {string} milestoneId 
     * @param {string} name 
     * @param {string} description 
     * @returns {Promise<Object>}
     */
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
    
    /**
     * Get resources for a subject
     * @param {string} subjectId 
     * @returns {Promise<Array>}
     */
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
    
    /**
     * Add a resource to a subject
     * @param {string} subjectId 
     * @param {string} name 
     * @param {string} type 
     * @param {string} url 
     * @returns {Promise<Object>}
     */
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
