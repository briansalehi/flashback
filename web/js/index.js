/**
 * Login page script
 */

// Wait for everything to load
window.addEventListener('DOMContentLoaded', () => {
    // Redirect if already authenticated
    Auth.redirectIfAuthenticated();
    
    // Handle form submission
    document.getElementById('signin-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;
        
        UI.hideMessage('error-message');
        UI.setButtonLoading('signin-btn', true);
        
        try {
            const result = await flashbackClient.signIn(email, password);
            console.log('Sign in successful:', result);
            
            // Redirect to home
            window.location.href = '/home.html';
        } catch (err) {
            console.error('Sign in failed:', err);
            UI.showError(err.message || 'Sign in failed. Please check your credentials.');
            UI.setButtonLoading('signin-btn', false);
        }
    });
});
