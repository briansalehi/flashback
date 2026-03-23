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
    const openResetPasswordModalBtn = document.getElementById('open-reset-password-modal-btn');
    const resetPasswordModal = document.getElementById('reset-password-modal');
    const confirmResetPasswordBtn = document.getElementById('confirm-reset-password-btn');
    const cancelResetPasswordBtn = document.getElementById('cancel-reset-password-btn');
    const closeResetPasswordModalBtn = document.getElementById('close-reset-password-modal-btn');
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
        if (successMessage) {
            UI.showSuccess(message, 'success-message');
            successMessage.scrollIntoView({ behavior: 'smooth', block: 'center' });
            // Add a little pop animation class if we had one
            successMessage.style.animation = 'fadeInUp 0.3s ease-out';
        } else {
            alert(message);
        }
        setTimeout(() => {
            UI.hideMessage('success-message');
        }, 5000);
    }

    function showError(message) {
        if (errorMessage) {
            UI.showError(message, 'error-message');
        } else {
            alert('Error: ' + message);
        }
    }

    function hideMessages() {
        if (successMessage) successMessage.style.display = 'none';
        if (errorMessage) errorMessage.style.display = 'none';
    }

    let originalData = { name: '', email: '' };

    function toggleEditMode(editing) {
        if (editing) {
            if (userNameInput) originalData.name = userNameInput.value;
            if (userEmailInput) originalData.email = userEmailInput.value;
            
            if (userNameInput) userNameInput.removeAttribute('readonly');
            if (userEmailInput) userEmailInput.removeAttribute('readonly');
            if (userNameInput) userNameInput.focus();
            
            if (editModeBtn) editModeBtn.style.display = 'none';
            if (editActions) editActions.style.display = 'flex';
        } else {
            if (userNameInput) userNameInput.setAttribute('readonly', true);
            if (userEmailInput) userEmailInput.setAttribute('readonly', true);
            
            if (editModeBtn) editModeBtn.style.display = 'block';
            if (editActions) editActions.style.display = 'none';
        }
    }

    if (editModeBtn) {
        editModeBtn.addEventListener('click', () => toggleEditMode(true));
    }
    
    if (cancelEditBtn) {
        cancelEditBtn.addEventListener('click', () => {
            if (userNameInput) userNameInput.value = originalData.name;
            if (userEmailInput) userEmailInput.value = originalData.email;
            toggleEditMode(false);
        });
    }

    async function loadUser() {
        try {
            const user = await client.getUser();
            if (userNameInput) userNameInput.value = user.name || '';
            if (userEmailInput) userEmailInput.value = user.email || '';
            if (userJoinedInput) {
                if (user.joined !== undefined && user.joined !== null) {
                    userJoinedInput.value = UI.formatISODate(user.joined * 1000);
                } else {
                    userJoinedInput.value = '';
                }
            }

            // Update verification status
            if (user.isVerified) {
                if (verificationBadge) {
                    verificationBadge.textContent = 'Verified';
                    verificationBadge.style.display = 'inline-flex';
                    verificationBadge.style.background = 'rgba(76, 175, 80, 0.2)';
                    verificationBadge.style.color = '#4caf50';
                }
                if (verificationSection) verificationSection.style.display = 'none';
            } else {
                if (verificationBadge) {
                    verificationBadge.textContent = 'Unverified';
                    verificationBadge.style.display = 'inline-flex';
                    verificationBadge.style.background = 'rgba(244, 67, 54, 0.2)';
                    verificationBadge.style.color = '#f44336';
                }
                if (verificationSection) verificationSection.style.display = 'block';
            }
        } catch (err) {
            console.error('Load user error:', err);
            showError('Failed to load user information.');
        }
    }

    loadUser();

    if (editUserForm) {
        editUserForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            hideMessages();

            const name = userNameInput ? userNameInput.value.trim() : '';
            const email = userEmailInput ? userEmailInput.value.trim() : '';

            if (!name || !email) {
                showError('Both name and email are required.');
                return;
            }

            try {
                if (saveUserBtn) {
                    saveUserBtn.disabled = true;
                    saveUserBtn.textContent = 'Saving...';
                }

                await client.editUser(name, email);

                showSuccess('User information updated successfully!');
                toggleEditMode(false);
                
                // Refresh user data to update verification status if email changed
                await loadUser();

                if (saveUserBtn) {
                    saveUserBtn.disabled = false;
                    saveUserBtn.textContent = 'Save Changes';
                }
            } catch (err) {
                console.error('Edit user error:', err);
                showError(err.message || 'Failed to update user information. Please try again.');
                if (saveUserBtn) {
                    saveUserBtn.disabled = false;
                    saveUserBtn.textContent = 'Save Changes';
                }
            }
        });
    }

    // Handle Reset Password Modal
    function openResetPasswordModal() {
        if (resetPasswordModal) {
            resetPasswordModal.style.display = 'flex';
            document.body.style.overflow = 'hidden';
        }
    }

    function closeResetPasswordModal() {
        if (resetPasswordModal) {
            resetPasswordModal.style.display = 'none';
            document.body.style.overflow = '';
        }
    }

    if (openResetPasswordModalBtn) {
        openResetPasswordModalBtn.addEventListener('click', openResetPasswordModal);
    }

    if (cancelResetPasswordBtn) {
        cancelResetPasswordBtn.addEventListener('click', closeResetPasswordModal);
    }

    if (closeResetPasswordModalBtn) {
        closeResetPasswordModalBtn.addEventListener('click', closeResetPasswordModal);
    }

    if (resetPasswordModal) {
        resetPasswordModal.addEventListener('click', (e) => {
            if (e.target === resetPasswordModal) {
                closeResetPasswordModal();
            }
        });
    }

    if (confirmResetPasswordBtn) {
        confirmResetPasswordBtn.addEventListener('click', async () => {
            hideMessages();
            closeResetPasswordModal();

            try {
                UI.setButtonLoading('open-reset-password-modal-btn', true);
                await client.resetPassword();
                UI.setButtonLoading('open-reset-password-modal-btn', false);
                showSuccess('Password reset successfully! Check your email for the new password.');
            } catch (err) {
                console.error('Reset password error:', err);
                UI.setButtonLoading('open-reset-password-modal-btn', false);
                showError(err.message || 'Failed to reset password. Please try again.');
            }
        });
    }

    // Handle Verification
    let verificationTimer = null;
    let secondsRemaining = 0;

    function startVerificationTimer() {
        secondsRemaining = 60;
        if (sendVerificationBtn) sendVerificationBtn.disabled = true;
        
        if (verificationTimer) clearInterval(verificationTimer);
        
        verificationTimer = setInterval(() => {
            secondsRemaining--;
            if (secondsRemaining <= 0) {
                clearInterval(verificationTimer);
                if (sendVerificationBtn) {
                    sendVerificationBtn.disabled = false;
                    sendVerificationBtn.textContent = 'Send Verification Email';
                }
            } else {
                if (sendVerificationBtn) {
                    sendVerificationBtn.textContent = `Resend in ${secondsRemaining}s`;
                }
            }
        }, 1000);
    }

    if (sendVerificationBtn) {
        sendVerificationBtn.addEventListener('click', async () => {
            hideMessages();
            UI.setButtonLoading('send-verification-btn', true);

            try {
                await client.sendVerification();
                UI.setButtonLoading('send-verification-btn', false);
                showSuccess('Verification email sent! Please check your inbox.');
                if (verifyCodeContainer) verifyCodeContainer.style.display = 'block';
                startVerificationTimer();
                if (verificationCodeInput) verificationCodeInput.focus();
            } catch (err) {
                console.error('Send verification error:', err);
                UI.setButtonLoading('send-verification-btn', false);
                showError(err.message || 'Failed to send verification email. Please try again.');
            }
        });
    }

    if (verifyUserBtn) {
        verifyUserBtn.addEventListener('click', async () => {
            hideMessages();
            const code = verificationCodeInput ? verificationCodeInput.value.trim() : '';
            
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
                if (verifyCodeContainer) verifyCodeContainer.style.display = 'none';
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
    }

    if (signoutBtn) {
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
    }
});
