window.addEventListener('DOMContentLoaded', () => {
    Auth.redirectIfAuthenticated();

    document.getElementById('signin-form').addEventListener('submit', async (e) => {
        e.preventDefault();

        const email = document.getElementById('email').value;
        const password = document.getElementById('password').value;

        UI.hideMessage('error-message');
        UI.setButtonLoading('signin-btn', true);

        try {
            await client.signIn(email, password);
            window.location.href = '/home.html';
        } catch (err) {
            console.error('Sign in failed:', err);
            UI.showError(err.message || 'Please check your credentials.');
            UI.setButtonLoading('signin-btn', false);
        }
    });
});