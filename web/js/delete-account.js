document.addEventListener('DOMContentLoaded', () => {
    const params = new URLSearchParams(window.location.search);
    const code = parseInt(params.get('code'), 10);

    if (!code) {
        window.location.href = 'account.html';
        return;
    }

    if (!client.isAuthenticated()) {
        window.location.href = 'index.html';
        return;
    }

    const confirmDeleteBtn = document.getElementById('confirm-delete-btn');
    const confirmEmailInput = document.getElementById('confirm-email');
    const errorEl = document.getElementById('error-message');

    confirmDeleteBtn.addEventListener('click', async () => {
        const email = confirmEmailInput ? confirmEmailInput.value.trim() : '';

        if (!email) {
            errorEl.textContent = 'Please enter your email address.';
            errorEl.style.display = 'block';
            return;
        }

        errorEl.style.display = 'none';
        confirmDeleteBtn.disabled = true;
        confirmDeleteBtn.textContent = 'Deleting...';

        try {
            await client.deleteAccount(email, code);
            window.location.href = 'index.html';
        } catch (err) {
            errorEl.textContent = err || 'Failed to delete account. Please try again.';
            errorEl.style.display = 'block';
            confirmDeleteBtn.disabled = false;
            confirmDeleteBtn.textContent = 'Yes, Delete My Account';
        }
    });
});
