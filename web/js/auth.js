const Auth = {
    isAuthenticated() {
        return !!localStorage.getItem('token');
    },
    
    getToken() {
        return localStorage.getItem('token');
    },
    
    requireAuth() {
        if (!this.isAuthenticated()) {
            window.location.href = '/index.html';
            return false;
        }
        return true;
    },
    
    redirectIfAuthenticated() {
        if (this.isAuthenticated()) {
            window.location.href = '/home.html';
        }
    },
    
    async signOut() {
        try {
            await flashbackClient.signOut();
        } catch (err) {
            console.error('Sign out error:', err);
        }
        localStorage.removeItem('token');
        window.location.href = '/index.html';
    }
};
