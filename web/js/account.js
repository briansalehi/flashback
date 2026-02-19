// Account Management JavaScript

document.addEventListener('DOMContentLoaded', () => {
    // Check authentication
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    // Get DOM elements
    const editUserForm = document.getElementById('edit-user-form');
    const userNameInput = document.getElementById('user-name');
    const userEmailInput = document.getElementById('user-email');
    const saveUserBtn = document.getElementById('save-user-btn');
    const resetPasswordBtn = document.getElementById('reset-password-btn');
    const signoutBtn = document.getElementById('signout-btn');
    const successMessage = document.getElementById('success-message');
    const errorMessage = document.getElementById('error-message');

    // Helper functions for showing messages
    function showSuccess(message) {
        successMessage.textContent = message;
        successMessage.style.display = 'block';
        errorMessage.style.display = 'none';
        setTimeout(() => {
            successMessage.style.display = 'none';
        }, 5000);
    }

    function showError(message) {
        errorMessage.textContent = message;
        errorMessage.style.display = 'block';
        successMessage.style.display = 'none';
    }

    function hideMessages() {
        successMessage.style.display = 'none';
        errorMessage.style.display = 'none';
    }

    // Handle Edit User Form submission
    editUserForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        hideMessages();

        const name = userNameInput.value.trim();
        const email = userEmailInput.value.trim();

        if (!name || !email) {
            showError('Both name and email are required.');
            return;
        }

        try {
            saveUserBtn.disabled = true;
            saveUserBtn.textContent = 'Saving...';

            await client.editUser(name, email);

            showSuccess('User information updated successfully!');

            saveUserBtn.disabled = false;
            saveUserBtn.textContent = 'Save Changes';
        } catch (err) {
            console.error('Edit user error:', err);
            showError(err.message || 'Failed to update user information. Please try again.');
            saveUserBtn.disabled = false;
            saveUserBtn.textContent = 'Save Changes';
        }
    });

    // Handle Reset Password
    resetPasswordBtn.addEventListener('click', async () => {
        hideMessages();

        const confirmed = confirm('Are you sure you want to reset your password? A new password will be sent to your email.');
        if (!confirmed) {
            return;
        }

        try {
            resetPasswordBtn.disabled = true;
            resetPasswordBtn.textContent = 'Resetting...';

            await client.resetPassword();

            showSuccess('Password reset successfully! Check your email for the new password.');

            resetPasswordBtn.disabled = false;
            resetPasswordBtn.textContent = 'Reset Password';
        } catch (err) {
            console.error('Reset password error:', err);
            showError(err.message || 'Failed to reset password. Please try again.');
            resetPasswordBtn.disabled = false;
            resetPasswordBtn.textContent = 'Reset Password';
        }
    });

    // Handle Sign Out
    signoutBtn.addEventListener('click', async (e) => {
        e.preventDefault();
        try {
            await client.signOut();
            window.location.href = '/home.html';
        } catch (err) {
            console.error('Sign out error:', err);
            window.location.href = '/home.html';
        }
    });
});
