/**
 * Authentication utilities
 */

const Auth = {
    /**
     * Check if user is authenticated
     */
    isAuthenticated() {
        return !!sessionStorage.getItem('authToken');
    },
    
    /**
     * Get current auth token
     */
    getToken() {
        return sessionStorage.getItem('authToken');
    },
    
    /**
     * Redirect to login if not authenticated
     */
    requireAuth() {
        if (!this.isAuthenticated()) {
            window.location.href = '/index.html';
            return false;
        }
        return true;
    },
    
    /**
     * Redirect to home if already authenticated
     */
    redirectIfAuthenticated() {
        if (this.isAuthenticated()) {
            window.location.href = '/home.html';
        }
    },
    
    /**
     * Sign out and redirect to login
     */
    async signOut() {
        try {
            await flashbackClient.signOut();
        } catch (err) {
            console.error('Sign out error:', err);
        }
        sessionStorage.removeItem('authToken');
        window.location.href = '/index.html';
    }
};
