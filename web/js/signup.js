/**
 * Signup page script
 */

window.addEventListener('DOMContentLoaded', () => {
    // Redirect if already authenticated
    Auth.redirectIfAuthenticated();
    
    // Handle form submission
    document.getElementById('signup-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const displayName = document.getElementById('display-name').value;
        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirm-password').value;
        
        UI.hideMessage('error-message');
        
        // Validate passwords match
        if (password !== confirmPassword) {
            UI.showError('Passwords do not match');
            return;
        }
        
        // Validate password strength
        if (password.length < 8) {
            UI.showError('Password must be at least 8 characters long');
            return;
        }
        
        UI.setButtonLoading('signup-btn', true);
        
        try {
            const result = await flashbackClient.signUp(email, password, displayName);
            console.log('Sign up successful:', result);
            
            // Redirect to home
            window.location.href = '/home.html';
        } catch (err) {
            console.error('Sign up failed:', err);
            UI.showError(err.message || 'Sign up failed. Email may already be in use.');
            UI.setButtonLoading('signup-btn', false);
        }
    });
});
