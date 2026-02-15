document.getElementById('signup-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    const name = document.getElementById('name').value;
    const email = document.getElementById('email').value;
    const password = document.getElementById('password').value;
    const confirmPassword = document.getElementById('confirm-password').value;

    UI.hideMessage('error-message');

    if (password !== confirmPassword) {
        UI.showError('Passwords do not match');
        return;
    }

    if (password.length < 4) {
        UI.showError('Password must be at least 4 characters long');
        return;
    }

    UI.setButtonLoading('signup-btn', true);

    try {
        await client.signUp(name, email, password);
        window.location.href = '/home.html';
    } catch (err) {
        console.error('Sign up failed:', err);
        UI.showError(err.message || 'Email may already be in use');
        UI.setButtonLoading('signup-btn', false);
    }
});
