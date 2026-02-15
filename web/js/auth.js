const Auth = {
    isAuthenticated() {
        return !!localStorage.getItem('token');
    },
    
    getToken() {
        return localStorage.getItem('token');
    },
    
    redirectIfAuthenticated() {
        if (this.isAuthenticated()) {
            window.location.href = '/home.html';
        }
    },
    
    async signOut() {
        try {
            await client.signOut();
        } catch (err) {
            console.error('Sign out error:', err);
        }
        localStorage.removeItem('token');
        window.location.href = '/index.html';
    }
};
