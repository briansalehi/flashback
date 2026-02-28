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
    const userJoinedInput = document.getElementById('user-joined');
    const editModeBtn = document.getElementById('edit-mode-btn');
    const editActions = document.getElementById('edit-actions');
    const cancelEditBtn = document.getElementById('cancel-edit-btn');
    const saveUserBtn = document.getElementById('save-user-btn');
    const resetPasswordBtn = document.getElementById('reset-password-btn');
    const signoutBtn = document.getElementById('signout-btn');
    const successMessage = document.getElementById('success-message');
    const errorMessage = document.getElementById('error-message');

    // Verification elements
    const verificationBadge = document.getElementById('verification-badge');
    const verificationSection = document.getElementById('verification-section');
    const sendVerificationBtn = document.getElementById('send-verification-btn');
    const verifyCodeContainer = document.getElementById('verify-code-container');
    const verificationCodeInput = document.getElementById('verification-code');
    const verifyUserBtn = document.getElementById('verify-user-btn');

    function showSuccess(message) {
        UI.showSuccess(message, 'success-message');
        const successEl = document.getElementById('success-message');
        if (successEl) {
            successEl.scrollIntoView({ behavior: 'smooth', block: 'center' });
            // Add a little pop animation class if we had one
            successEl.style.animation = 'fadeInUp 0.3s ease-out';
        }
        setTimeout(() => {
            UI.hideMessage('success-message');
        }, 5000);
    }

    function showError(message) {
        UI.showError(message, 'error-message');
    }

    function hideMessages() {
        successMessage.style.display = 'none';
        errorMessage.style.display = 'none';
    }

    let originalData = { name: '', email: '' };

    function toggleEditMode(editing) {
        if (editing) {
            originalData.name = userNameInput.value;
            originalData.email = userEmailInput.value;
            
            userNameInput.removeAttribute('readonly');
            userEmailInput.removeAttribute('readonly');
            userNameInput.focus();
            
            editModeBtn.style.display = 'none';
            editActions.style.display = 'flex';
        } else {
            userNameInput.setAttribute('readonly', true);
            userEmailInput.setAttribute('readonly', true);
            
            editModeBtn.style.display = 'block';
            editActions.style.display = 'none';
        }
    }

    editModeBtn.addEventListener('click', () => toggleEditMode(true));
    
    cancelEditBtn.addEventListener('click', () => {
        userNameInput.value = originalData.name;
        userEmailInput.value = originalData.email;
        toggleEditMode(false);
    });

    async function loadUser() {
        try {
            const user = await client.getUser();
            userNameInput.value = user.name || '';
            userEmailInput.value = user.email || '';
            if (user.joined !== undefined && user.joined !== null) {
                userJoinedInput.value = UI.formatISODate(user.joined * 1000);
            } else {
                userJoinedInput.value = '';
            }

            // Update verification status
            if (user.isVerified) {
                verificationBadge.textContent = 'Verified';
                verificationBadge.style.display = 'inline-flex';
                verificationBadge.style.background = 'rgba(76, 175, 80, 0.2)';
                verificationBadge.style.color = '#4caf50';
                verificationSection.style.display = 'none';
            } else {
                verificationBadge.textContent = 'Unverified';
                verificationBadge.style.display = 'inline-flex';
                verificationBadge.style.background = 'rgba(244, 67, 54, 0.2)';
                verificationBadge.style.color = '#f44336';
                verificationSection.style.display = 'block';
            }
        } catch (err) {
            console.error('Load user error:', err);
            showError('Failed to load user information.');
        }
    }

    loadUser();

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
            toggleEditMode(false);

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

    // Handle Verification
    let verificationTimer = null;
    let secondsRemaining = 0;

    function startVerificationTimer() {
        secondsRemaining = 60;
        sendVerificationBtn.disabled = true;
        
        if (verificationTimer) clearInterval(verificationTimer);
        
        verificationTimer = setInterval(() => {
            secondsRemaining--;
            if (secondsRemaining <= 0) {
                clearInterval(verificationTimer);
                sendVerificationBtn.disabled = false;
                sendVerificationBtn.textContent = 'Send Verification Email';
            } else {
                sendVerificationBtn.textContent = `Resend in ${secondsRemaining}s`;
            }
        }, 1000);
    }

    sendVerificationBtn.addEventListener('click', async () => {
        hideMessages();
        UI.setButtonLoading('send-verification-btn', true);

        try {
            await client.sendVerification();
            UI.setButtonLoading('send-verification-btn', false);
            showSuccess('Verification email sent! Please check your inbox.');
            verifyCodeContainer.style.display = 'block';
            startVerificationTimer();
            verificationCodeInput.focus();
        } catch (err) {
            console.error('Send verification error:', err);
            UI.setButtonLoading('send-verification-btn', false);
            showError(err.message || 'Failed to send verification email. Please try again.');
        }
    });

    verifyUserBtn.addEventListener('click', async () => {
        hideMessages();
        const code = verificationCodeInput.value.trim();
        
        if (code.length !== 6 || isNaN(code)) {
            showError('Please enter a valid 6-digit verification code.');
            return;
        }

        UI.setButtonLoading('verify-user-btn', true);

        try {
            await client.verifyUser(parseInt(code));
            UI.setButtonLoading('verify-user-btn', false);
            showSuccess('Email verified successfully!');
            
            // Clean up UI
            verifyCodeContainer.style.display = 'none';
            if (verificationTimer) {
                clearInterval(verificationTimer);
                verificationTimer = null;
            }
            
            // Reload user to update badge
            await loadUser();
        } catch (err) {
            console.error('Verify user error:', err);
            UI.setButtonLoading('verify-user-btn', false);
            showError(err.message || 'Invalid verification code. Please try again.');
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
