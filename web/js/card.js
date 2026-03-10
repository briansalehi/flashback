// Timer to track card reading duration
let cardStartTime = null;

async function markCardAsReviewed() {
    const cardId = parseInt(UI.getUrlParam('cardId'));
    const markReviewedBtn = document.getElementById('mark-reviewed-btn');

    if (!cardId) {
        UI.showError('Invalid card ID');
        return;
    }

    UI.setButtonLoading('mark-reviewed-btn', true);

    try {
        await client.markCardAsReviewed(cardId);

        // Update state display
        const stateElement = document.getElementById('card-state');
        if (stateElement) {
            stateElement.textContent = 'reviewed';
            stateElement.className = 'card-state-display reviewed';
            stateElement.style.display = 'inline-block';
        }

        // Update URL state parameter to 1 (reviewed)
        const url = new URL(window.location.href);
        url.searchParams.set('state', '1');
        window.history.replaceState({}, '', url);

        // Hide the button
        if (markReviewedBtn) {
            markReviewedBtn.style.display = 'none';
        }

        UI.setButtonLoading('mark-reviewed-btn', false);
    } catch (err) {
        console.error('Failed to mark card as reviewed:', err);
        UI.showError('Failed to mark card as reviewed: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('mark-reviewed-btn', false);
    }
}

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const headline = UI.getUrlParam('headline');
    const state = parseInt(UI.getUrlParam('state')) || 0;
    const cardIndex = parseInt(UI.getUrlParam('cardIndex'));
    const totalCards = parseInt(UI.getUrlParam('totalCards'));
    const subjectName = UI.getUrlParam('subjectName');
    const topicName = UI.getUrlParam('topicName');
    const resourceName = UI.getUrlParam('resourceName');
    const sectionName = UI.getUrlParam('sectionName');

    if (isNaN(cardId)) {
        window.location.href = '/home.html';
        return;
    }

    // Start timer
    cardStartTime = Date.now();

    // Global click-to-hide listener for block actions
    document.addEventListener('mousedown', (e) => {
        const blocks = document.querySelectorAll('.content-block.show-actions');
        blocks.forEach(blockItem => {
            if (!blockItem.contains(e.target) && !e.target.closest('.block-actions-overlay')) {
                blockItem.classList.remove('show-actions');
            }
        });
    });

    // Show context breadcrumb
    displayContextBreadcrumb(subjectName, topicName, resourceName, sectionName);

    const headlineEl = document.getElementById('card-headline');
    if (headlineEl) {
        headlineEl.textContent = headline || 'Card';
        headlineEl.setAttribute('data-original-text', headline || 'Card');
        if (typeof renderMathInElement !== 'undefined') {
            renderMathInElement(headlineEl, {
                delimiters: [
                    {left: '$$', right: '$$', display: true},
                    {left: '$', right: '$', display: false}
                ],
                throwOnError: false
            });
        }
    }
    document.title = `${headline || 'Card'} - Flashback`;

    const stateNames = ['draft', 'reviewed', 'completed', 'approved', 'released', 'rejected'];
    const stateName = stateNames[state] || 'draft';
    const stateElement = document.getElementById('card-state');
    if (stateElement) {
        stateElement.textContent = stateName;
        stateElement.className = `card-state-display ${stateName}`;
        stateElement.style.display = 'inline-block';
    }

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await handleCardExit();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    // Add navigation based on mode
    if (!isNaN(cardIndex) && !isNaN(totalCards)) {
        // Full practice session navigation
        addPracticeNavigation(cardIndex, totalCards);
    } else if ((UI.getUrlParam('practiceMode') || '') === 'selective') {
        // Selective navigation when user opened a card from topic-cards or section-cards
        setupSelectiveNavigation();
    } else {
        // Simple study mode
        addStudyNavigation();
    }

    // Track when user navigates away from card
    // Intercept link clicks to properly record progress before navigation
    document.addEventListener('click', async (e) => {
        const link = e.target.closest('a');
        if (link && link.href && cardStartTime) {
            e.preventDefault();
            await handleCardExit();
            window.location.href = link.href;
        }
    });

    // Setup mark as reviewed button (reveal if in draft state)
    const markReviewedBtn = document.getElementById('mark-reviewed-btn');
    if (markReviewedBtn) {
        // Reveal by default if in draft state (state 0)
        if (state === 0) {
            markReviewedBtn.style.display = 'inline-block';
        } else {
            markReviewedBtn.style.display = 'none';
        }

        const openReviewModal = () => {
            const modal = document.getElementById('review-confirmation-modal');
            if (modal) {
                modal.style.display = 'flex';
                document.body.style.overflow = 'hidden';
            }
        };

        const closeReviewModal = () => {
            const modal = document.getElementById('review-confirmation-modal');
            if (modal) {
                modal.style.display = 'none';
                document.body.style.overflow = '';
            }
        };

        markReviewedBtn.addEventListener('click', (e) => {
            e.preventDefault();
            openReviewModal();
        });

        const cancelReviewBtn = document.getElementById('cancel-review-btn');
        if (cancelReviewBtn) cancelReviewBtn.addEventListener('click', closeReviewModal);

        const closeReviewModalBtn = document.getElementById('close-review-modal-btn');
        if (closeReviewModalBtn) closeReviewModalBtn.addEventListener('click', closeReviewModal);

        const reviewModal = document.getElementById('review-confirmation-modal');
        if (reviewModal) {
            reviewModal.addEventListener('click', (e) => {
                if (e.target === reviewModal) closeReviewModal();
            });
        }

        const confirmReviewBtn = document.getElementById('confirm-review-btn');
        if (confirmReviewBtn) {
            confirmReviewBtn.addEventListener('click', async () => {
                await markCardAsReviewed();
                closeReviewModal();
            });
        }
    }

    // Also defensively hide Edit and Remove until revealed
    const editBtnInit = document.getElementById('edit-headline-btn');
    if (editBtnInit) editBtnInit.style.display = 'none';
    const removeBtnInit = document.getElementById('remove-card-btn');
    if (removeBtnInit) removeBtnInit.style.display = 'none';

    // Reveal header actions on headline click
    if (headlineEl) {
        headlineEl.setAttribute('tabindex', '0');
        const editBtn = document.getElementById('edit-headline-btn');
        const removeBtn = document.getElementById('remove-card-btn');
        const reveal = () => {
            if (editBtn) editBtn.style.display = 'inline-block';
            if (removeBtn) removeBtn.style.display = 'inline-block';
            if (markReviewedBtn && state !== 1) markReviewedBtn.style.display = 'inline-block';
        };
        headlineEl.addEventListener('click', reveal);
        headlineEl.addEventListener('keydown', (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); reveal(); }});
    }

    const adjustTextareaHeight = (el) => {
        el.style.height = 'auto';
        el.style.height = el.scrollHeight + 'px';
    };

    // Edit Card Modal Functions
    const openEditCardModal = () => {
        const modal = document.getElementById('edit-card-modal');
        if (modal) {
            modal.style.display = 'flex';
            document.body.style.overflow = 'hidden';
            const headlineEl = document.getElementById('card-headline');
            const headlineInput = document.getElementById('edit-card-headline');
            if (headlineInput) {
                headlineInput.value = headlineEl ? (headlineEl.getAttribute('data-original-text') || headlineEl.textContent) : '';
                setTimeout(() => {
                    headlineInput.focus();
                    adjustTextareaHeight(headlineInput);
                }, 100);
            }
        }
    };

    const closeEditCardModal = () => {
        const modal = document.getElementById('edit-card-modal');
        if (modal) {
            modal.style.display = 'none';
            document.body.style.overflow = '';
            UI.clearForm('edit-card-form');
        }
    };

    const editCardBtn = document.getElementById('edit-headline-btn');
    if (editCardBtn) {
        editCardBtn.addEventListener('click', (e) => {
            e.preventDefault();
            openEditCardModal();
        });
    }

    const cancelEditCardBtn = document.getElementById('cancel-edit-card-btn');
    if (cancelEditCardBtn) cancelEditCardBtn.addEventListener('click', closeEditCardModal);

    const closeEditCardModalBtn = document.getElementById('close-edit-card-modal-btn');
    if (closeEditCardModalBtn) closeEditCardModalBtn.addEventListener('click', closeEditCardModal);

    const editCardModal = document.getElementById('edit-card-modal');
    if (editCardModal) {
        editCardModal.addEventListener('click', (e) => {
            if (e.target === editCardModal) closeEditCardModal();
        });
    }

    const editCardHeadlineInput = document.getElementById('edit-card-headline');
    if (editCardHeadlineInput) {
        editCardHeadlineInput.addEventListener('input', () => {
            adjustTextareaHeight(editCardHeadlineInput);
        });
    }

    const editCardForm = document.getElementById('edit-card-form');
    if (editCardForm) {
        editCardForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const newHeadline = document.getElementById('edit-card-headline').value.trim();
            if (!newHeadline) {
                UI.showError('Headline cannot be empty');
                return;
            }

            UI.setButtonLoading('save-edit-card-btn', true);
            try {
                const cardId = parseInt(UI.getUrlParam('cardId'));
                await client.editCard(cardId, newHeadline);
                const headlineEl = document.getElementById('card-headline');
                headlineEl.textContent = newHeadline;
                headlineEl.setAttribute('data-original-text', newHeadline);
                if (typeof renderMathInElement !== 'undefined') {
                    renderMathInElement(headlineEl, {
                        delimiters: [
                            {left: '$$', right: '$$', display: true},
                            {left: '$', right: '$', display: false}
                        ],
                        throwOnError: false
                    });
                }
                document.title = `${newHeadline} - Flashback`;
                closeEditCardModal();
                UI.setButtonLoading('save-edit-card-btn', false);
                UI.showSuccess('Card updated successfully');
            } catch (err) {
                console.error('Failed to edit card headline:', err);
                UI.showError('Failed to edit headline: ' + (err.message || 'Unknown error'));
                UI.setButtonLoading('save-edit-card-btn', false);
            }
        });
    }

    // Setup remove card button (attach handler only; reveal on title click)
    const removeCardBtn = document.getElementById('remove-card-btn');
    const removeCardModal = document.getElementById('remove-card-modal');
    const closeRemoveCardModalBtn = document.getElementById('close-remove-card-modal-btn');
    if (removeCardBtn) {
        removeCardBtn.addEventListener('click', (e) => {
            e.preventDefault();
            openRemoveCardModal();
        });
    }

    if (closeRemoveCardModalBtn) closeRemoveCardModalBtn.addEventListener('click', closeRemoveCardModal);
    if (removeCardModal) {
        removeCardModal.addEventListener('click', (e) => {
            if (e.target === removeCardModal) closeRemoveCardModal();
        });
    }

    // Setup remove card modal handlers
    const cancelRemoveCardBtn = document.getElementById('cancel-remove-card-btn');
    if (cancelRemoveCardBtn) {
        cancelRemoveCardBtn.addEventListener('click', () => {
            closeRemoveCardModal();
        });
    }

    const confirmRemoveCardBtn = document.getElementById('confirm-remove-card-btn');
    if (confirmRemoveCardBtn) {
        confirmRemoveCardBtn.addEventListener('click', async () => {
            await confirmRemoveCard();
        });
    }

    // Setup remove block modal handlers
    const removeBlockModal = document.getElementById('remove-block-modal');
    const closeRemoveBlockModalBtn = document.getElementById('close-remove-block-modal-btn');
    const cancelRemoveBlockBtn = document.getElementById('cancel-remove-block-btn');
    if (cancelRemoveBlockBtn) {
        cancelRemoveBlockBtn.addEventListener('click', () => {
            closeRemoveBlockModal();
        });
    }

    if (closeRemoveBlockModalBtn) closeRemoveBlockModalBtn.addEventListener('click', closeRemoveBlockModal);
    if (removeBlockModal) {
        removeBlockModal.addEventListener('click', (e) => {
            if (e.target === removeBlockModal) closeRemoveBlockModal();
        });
    }

    const confirmRemoveBlockBtn = document.getElementById('confirm-remove-block-btn');
    if (confirmRemoveBlockBtn) {
        confirmRemoveBlockBtn.addEventListener('click', async () => {
            const modal = document.getElementById('remove-block-modal');
            const position = parseInt(modal.dataset.blockPosition);
            if (!isNaN(position)) {
                await removeBlockHandler(position);
            }
        });
    }

    const modalOverlay = document.getElementById('modal-overlay');

    // Add Block Modal Functions
    const openAddBlockModal = () => {
        UI.toggleElement('add-block-modal', true);
        document.body.style.overflow = 'hidden';

        // Set initial state for extension field based on default type (Text)
        const typeSelect = document.getElementById('block-type');
        const extensionInput = document.getElementById('block-extension');
        if (typeSelect && extensionInput) {
            const type = parseInt(typeSelect.value);
            if (type === 0) { // Text
                extensionInput.value = 'md';
                extensionInput.disabled = true;
            } else if (type === 3) { // Math
                extensionInput.value = 'tex';
                extensionInput.disabled = true;
            } else if (type === 4) { // Diagram
                extensionInput.value = 'json';
                extensionInput.disabled = true;
            } else {
                extensionInput.disabled = false;
            }
        }

        setTimeout(() => {
            const contentInput = document.getElementById('block-content');
            if (contentInput) contentInput.focus();
        }, 100);
    };

    // Add event listener for block type changes in Add Block modal
    const blockTypeSelect = document.getElementById('block-type');
    if (blockTypeSelect) {
        blockTypeSelect.addEventListener('change', () => {
            const extensionInput = document.getElementById('block-extension');
            if (extensionInput) {
                const type = parseInt(blockTypeSelect.value);
                if (type === 0) { // Text
                    extensionInput.value = 'md';
                    extensionInput.disabled = true;
                } else if (type === 3) { // Math
                    extensionInput.value = 'tex';
                    extensionInput.disabled = true;
                } else if (type === 4) { // Diagram
                    extensionInput.value = 'json';
                    extensionInput.disabled = true;
                } else {
                    extensionInput.disabled = false;
                }
            }
        });

        // Ensure extension is always lower case as user types
        const extensionInput = document.getElementById('block-extension');
        if (extensionInput) {
            extensionInput.addEventListener('input', () => {
                extensionInput.value = extensionInput.value.toLowerCase();
            });
        }
    }

    const closeAddBlockModal = () => {
        UI.toggleElement('add-block-modal', false);
        document.body.style.overflow = '';
        UI.clearForm('block-form');
    };

    if (modalOverlay) {
        modalOverlay.addEventListener('click', () => {
            closeRemoveCardModal();
            closeRemoveBlockModal();
            closeAddBlockModal();
        });
    }

    const addBlockModal = document.getElementById('add-block-modal');
    if (addBlockModal) {
        addBlockModal.addEventListener('click', (e) => {
            if (e.target === addBlockModal) {
                closeAddBlockModal();
            }
        });
    }

    const closeBlockModalBtn = document.getElementById('close-block-modal-btn');
    if (closeBlockModalBtn) {
        closeBlockModalBtn.addEventListener('click', closeAddBlockModal);
    }

    // Setup add block button
    const addBlockBtn = document.getElementById('add-block-btn');
    if (addBlockBtn) {
        addBlockBtn.addEventListener('click', (e) => {
            e.preventDefault();
            openAddBlockModal();
        });
    }

    const cancelBlockBtn = document.getElementById('cancel-block-btn');
    if (cancelBlockBtn) {
        cancelBlockBtn.addEventListener('click', closeAddBlockModal);
    }

    const blockForm = document.getElementById('block-form');
    if (blockForm) {
        blockForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const blockType = parseInt(document.getElementById('block-type').value);
            const extensionInput = document.getElementById('block-extension');
            const blockExtension = extensionInput.value.trim().toLowerCase();
            const blockMetadata = document.getElementById('block-metadata').value.trim();
            const blockContent = document.getElementById('block-content').value.trim();

            if (!blockContent) {
                UI.showError('Please enter block content');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-block-btn', true);

            try {
                await client.createBlock(cardId, blockType, blockExtension, blockContent, blockMetadata);

                closeAddBlockModal();
                UI.setButtonLoading('save-block-btn', false);

                loadBlocks();
            } catch (err) {
                console.error('Add block failed:', err);
                UI.showError(err.message || 'Failed to add block');
                UI.setButtonLoading('save-block-btn', false);
            }
        });
    }

    loadBlocks();
});

// Display context breadcrumb (subject/topic or resource/section)
function displayContextBreadcrumb(subjectName, topicName, resourceName, sectionName) {
    const breadcrumb = document.getElementById('context-breadcrumb');
    if (!breadcrumb) return;

    let contextHtml = '';
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const localSubjectName = UI.getUrlParam('subjectName') || subjectName || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || UI.getUrlParam('topicLevel') || UI.getUrlParam('level') || '0';
    const currentTab = UI.getUrlParam('tab') || (resourceName ? 'resources' : 'topics');

    if (subjectName && topicName) {
        // Get subject and roadmap info from practice state or URL
        const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');

        // Get topic info - prefer URL params over practice state to avoid using card position
        const currentTopic = practiceState.topics ? practiceState.topics[practiceState.currentTopicIndex] : null;
        const topicLevel = UI.getUrlParam('topicLevel') || (currentTopic ? currentTopic.level : '');
        const topicPosition = UI.getUrlParam('topicPosition') || (currentTopic ? currentTopic.position : '');

        let breadcrumbParts = [];

        if (roadmapId && roadmapName) {
            breadcrumbParts.push(`<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`);
        }

        if (subjectId && subjectName) {
            const subjectLink = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
            breadcrumbParts.push(`<a href="${subjectLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`);
        }

        if (subjectId && topicLevel !== '' && topicPosition !== '' && topicName) {
            const topicLink = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&name=${encodeURIComponent(topicName)}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&milestoneLevel=${milestoneLevel}`;
            breadcrumbParts.push(`<a href="${topicLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(topicName)}</a>`);
        }

        contextHtml = breadcrumbParts.join(' → ');
    } else {
        let breadcrumbParts = [];

        // Always add roadmap if available
        if (roadmapId && roadmapName) {
            breadcrumbParts.push(`<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`);
        }

        // Always add subject if available
        if (subjectId && localSubjectName) {
            const subjectLink = `subject.html?id=${subjectId}&name=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
            breadcrumbParts.push(`<a href="${subjectLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(localSubjectName)}</a>`);
        }

        // Check if coming from topic or resource/section
        const topicPosition = UI.getUrlParam('topicPosition');
        const topicLevel = UI.getUrlParam('topicLevel');
        const localTopicName = UI.getUrlParam('topicName') || topicName || '';

        if (topicPosition !== '' && topicLevel !== '' && localTopicName) {
            // Coming from topic-cards
            const topicLink = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&name=${encodeURIComponent(localTopicName)}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&milestoneLevel=${milestoneLevel}`;
            breadcrumbParts.push(`<a href="${topicLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(localTopicName)}</a>`);
        } else if (resourceName && sectionName) {
            // Coming from section-cards
            const resourceId = UI.getUrlParam('resourceId') || '';
            const sectionPosition = UI.getUrlParam('sectionPosition') || '';

            if (resourceId && resourceName) {
                const resourceType = UI.getUrlParam('resourceType') || '0';
                const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
                const resourceLink = UI.getUrlParam('resourceLink') || '';
                const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
                const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
                const resLink = `resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&type=${resourceType}&pattern=${resourcePattern}&link=${encodeURIComponent(resourceLink)}&production=${resourceProduction}&expiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
                breadcrumbParts.push(`<a href="${resLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(resourceName)}</a>`);
            }

            if (resourceId && sectionPosition !== '' && sectionName) {
                const resourceType = UI.getUrlParam('resourceType') || '0';
                const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
                const resourceLink = UI.getUrlParam('resourceLink') || '';
                const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
                const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
                const sectionState = UI.getUrlParam('sectionState') || '0';
                const sectionLink = `section-cards.html?resourceId=${resourceId}&sectionPosition=${sectionPosition}&sectionState=${sectionState}&name=${encodeURIComponent(sectionName)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink)}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
                breadcrumbParts.push(`<a href="${sectionLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(sectionName)}</a>`);
            }
        }

        contextHtml = breadcrumbParts.join(' → ');
    }

    if (contextHtml) {
        breadcrumb.innerHTML = contextHtml;
        breadcrumb.style.display = 'block';
    }
}

// Handle card exit - no longer records progress automatically
async function handleCardExit() {
    // Just reset the timer when exiting
    cardStartTime = null;
}

// Manual progress recording triggered by "Read" button (study mode only)
async function recordProgress(hideAfterSuccess = true) {
    if (!cardStartTime) {
        UI.showError('No reading session to record');
        return;
    }

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const duration = Math.floor((Date.now() - cardStartTime) / 1000); // Convert to seconds

    // Check if this is from a resource/section (study mode)
    const resourceId = UI.getUrlParam('resourceId');
    const sectionPosition = UI.getUrlParam('sectionPosition');

    UI.setButtonLoading('record-progress-btn', true);

    try {
        console.log(`studying card ${cardId} for ${duration} seconds`);
        await client.study(cardId, duration);
        UI.showSuccess('Study progress recorded!');

        // Hide button after successful recording in study mode
        if (hideAfterSuccess) {
            const readBtn = document.getElementById('record-progress-btn');
            if (readBtn) {
                readBtn.style.display = 'none';
            }
        }

        // Reset timer after successful recording
        cardStartTime = Date.now();
        UI.setButtonLoading('record-progress-btn', false);
    } catch (err) {
        console.error('Failed to record progress:', err);
        UI.showError('Failed to record progress: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('record-progress-btn', false);
    }
}

// Record practice progress and navigate
async function recordProgressAndNavigate(nextAction) {
    if (!cardStartTime) {
        // If no timer, just navigate
        await nextAction();
        return;
    }

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const duration = Math.floor((Date.now() - cardStartTime) / 1000);

    if (duration >= 3) {
        const milestoneId = UI.getUrlParam('milestoneId');
        const milestoneLevel = UI.getUrlParam('milestoneLevel');

        try {
            if (milestoneId && milestoneLevel !== undefined) {
                const parsedMilestoneId = parseInt(milestoneId);
                const parsedMilestoneLevel = parseInt(milestoneLevel);

                if (!isNaN(parsedMilestoneId) && !isNaN(parsedMilestoneLevel)) {
                    await client.makeProgress(cardId, duration, parsedMilestoneId, parsedMilestoneLevel);
                }
            }
        } catch (err) {
            console.error(`Failed to record progress in ${duration}:`, err);
        }
    }

    // Reset timer and navigate
    cardStartTime = null;
    await nextAction();
}

// Add navigation for study mode (non-practice)
function addStudyNavigation() {
    const contentDiv = document.getElementById('card-content');
    if (!contentDiv) return;

    const navContainer = document.createElement('div');
    navContainer.style.cssText = 'display: flex; justify-content: center; align-items: center; margin-top: 2rem; padding-top: 1rem; border-top: 2px solid var(--border-color);';

    const buttonContainer = document.createElement('div');
    buttonContainer.style.cssText = 'display: flex; gap: 1rem;';

    // Add "Read" button for recording study progress
    const readBtn = document.createElement('button');
    readBtn.textContent = 'Read';
    readBtn.id = 'record-progress-btn';
    readBtn.className = 'btn btn-primary';
    readBtn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
    readBtn.addEventListener('click', async () => {
        await recordProgress();
    });
    buttonContainer.appendChild(readBtn);

    navContainer.appendChild(buttonContainer);
    contentDiv.appendChild(navContainer);
}

// Build navigation for selective study (opened from topic-cards or section-cards)
async function setupSelectiveNavigation() {
    const contentDiv = document.getElementById('card-content');
    if (!contentDiv) return;

    const cardId = parseInt(UI.getUrlParam('cardId'));

    // Detect origin context
    const subjectId = parseInt(UI.getUrlParam('subjectId'));
    const topicPosition = parseInt(UI.getUrlParam('topicPosition'));
    const topicLevel = parseInt(UI.getUrlParam('topicLevel'));

    const resourceId = parseInt(UI.getUrlParam('resourceId'));
    const sectionPosition = parseInt(UI.getUrlParam('sectionPosition'));

    try {
        let cards = [];
        let currentIndex = -1;
        let contextType = '';

        if (!isNaN(subjectId) && !isNaN(topicPosition) && !isNaN(topicLevel)) {
            contextType = 'topic';
            cards = await client.getTopicCards(subjectId, topicPosition, topicLevel);
        } else if (!isNaN(resourceId) && !isNaN(sectionPosition)) {
            contextType = 'section';
            cards = await client.getSectionCards(resourceId, sectionPosition);
        }

        if (cards && cards.length) {
            currentIndex = cards.findIndex(c => parseInt(c.id) === cardId);
        }

        // Fallback to simple study nav if we cannot determine list/index
        if (currentIndex === -1) {
            addStudyNavigation();
            return;
        }

        await addSelectiveNavigation({
            contextType,
            cards,
            currentIndex
        });
    } catch (e) {
        console.error('Failed to setup selective navigation:', e);
        addStudyNavigation();
    }
}

// Add navigation for practice mode
// Record study (client.study) then perform next action
async function recordStudyAndNavigate(nextAction) {
    try {
        if (cardStartTime) {
            const cardId = parseInt(UI.getUrlParam('cardId'));
            const duration = Math.floor((Date.now() - cardStartTime) / 1000);
            if (!isNaN(cardId) && duration >= 3) {
                // Determine context: topic (makeProgress) vs section (study)
                const subjectId = parseInt(UI.getUrlParam('subjectId'));
                const topicPosition = parseInt(UI.getUrlParam('topicPosition'));
                const topicLevel = parseInt(UI.getUrlParam('topicLevel'));
                const resourceId = parseInt(UI.getUrlParam('resourceId'));
                const sectionPosition = parseInt(UI.getUrlParam('sectionPosition'));

                if (!isNaN(subjectId) && !isNaN(topicPosition) && !isNaN(topicLevel)) {
                    // Topic context → makeProgress
                    const milestoneLevelParam = UI.getUrlParam('milestoneLevel');
                    const milestoneLevel = !isNaN(parseInt(milestoneLevelParam)) ? parseInt(milestoneLevelParam) : topicLevel;
                    const milestoneId = subjectId; // In selective topic context, subjectId maps to milestone/subject id
                    await client.makeProgress(cardId, duration, milestoneId, milestoneLevel);
                } else if (!isNaN(resourceId) && !isNaN(sectionPosition)) {
                    // Section (chapter) context → study
                    await client.study(cardId, duration);
                } else {
                    // Fallback when context is unknown → study
                    await client.study(cardId, duration);
                }
            }
        }
    } catch (e) {
        console.error('Selective navigation record failed:', e);
        // Continue navigation regardless
    } finally {
        cardStartTime = null;
        await nextAction();
    }
}

function buildTopicCardUrl(card) {
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicPosition = UI.getUrlParam('topicPosition') || '';
    const topicLevel = UI.getUrlParam('topicLevel') || '';
    const topicName = UI.getUrlParam('topicName') || UI.getUrlParam('name') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel || UI.getUrlParam('level') || '0';
    return `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&practiceMode=selective&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&topicName=${encodeURIComponent(topicName)}&milestoneLevel=${milestoneLevel}`;
}

function buildSectionCardUrl(card) {
    const resourceName = UI.getUrlParam('resourceName') || '';
    const sectionName = UI.getUrlParam('sectionName') || UI.getUrlParam('name') || '';
    const resourceId = UI.getUrlParam('resourceId') || '';
    const sectionPosition = UI.getUrlParam('sectionPosition') || '';
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const sectionState = UI.getUrlParam('sectionState') || '0';
    return `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&sectionState=${sectionState}&practiceMode=selective&resourceName=${encodeURIComponent(resourceName)}&sectionName=${encodeURIComponent(sectionName)}&resourceId=${resourceId}&sectionPosition=${sectionPosition}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}`;
}

async function addSelectiveNavigation({ contextType, cards, currentIndex }) {
    const contentDiv = document.getElementById('card-content');
    if (!contentDiv) return;

    const navContainer = document.createElement('div');
    navContainer.style.cssText = 'display: flex; justify-content: space-between; align-items: center; margin-top: 2rem; padding-top: 1rem; border-top: 2px solid var(--border-color);';

    const infoText = document.createElement('span');
    infoText.textContent = `Card ${currentIndex + 1} of ${cards.length}`;
    infoText.style.color = 'var(--text-muted)';

    const buttonContainer = document.createElement('div');
    buttonContainer.style.cssText = 'display: flex; gap: 0.5rem; flex-wrap: wrap; justify-content: flex-end;';

    const isTopic = contextType === 'topic';

    const addNavButton = (label, isPrimary, onClick) => {
        const btn = document.createElement('button');
        btn.textContent = label;
        btn.className = 'btn btn-secondary';
        btn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        if (isPrimary) {
            btn.className = 'btn btn-primary';
            btn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        } else {
            btn.className = 'btn btn-secondary';
            btn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        }
        btn.addEventListener('click', async () => {
            await recordStudyAndNavigate(onClick);
        });
        buttonContainer.appendChild(btn);
    };

    // Helper for navigation
    const navigateToCard = (card) => {
        const url = isTopic ? buildTopicCardUrl(card) : buildSectionCardUrl(card);
        window.location.href = url;
    };

    try {
        if (isTopic) {
            const subjectId = parseInt(UI.getUrlParam('subjectId'));
            const topicLevel = parseInt(UI.getUrlParam('topicLevel'));
            const topicPosition = parseInt(UI.getUrlParam('topicPosition'));
            const roadmapId = UI.getUrlParam('roadmapId') || '';
            const roadmapName = UI.getUrlParam('roadmapName') || '';
            const subjectName = UI.getUrlParam('subjectName') || '';
            const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;

            const topics = await client.getTopics(subjectId, topicLevel);
            const sorted = topics.slice().sort((a, b) => a.position - b.position);
            const tIndex = sorted.findIndex(t => t.position === topicPosition);

            // Previous: Show Previous Topic only if at the first card of current topic
            if (currentIndex === 0 && tIndex > 0) {
                const prevTopic = sorted[tIndex - 1];
                addNavButton('Previous Topic', false, async () => {
                    window.location.href = `topic-cards.html?subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&topicPosition=${prevTopic.position}&topicLevel=${prevTopic.level}&name=${encodeURIComponent(prevTopic.name)}&milestoneLevel=${milestoneLevel}`;
                });
            } else if (currentIndex > 0) {
                addNavButton('Previous Card', false, async () => {
                    navigateToCard(cards[currentIndex - 1]);
                });
            }

            // Next: Show Next Topic only if at the last card of current topic
            if (currentIndex === cards.length - 1 && tIndex + 1 < sorted.length) {
                const nextTopic = sorted[tIndex + 1];
                addNavButton('Next Topic', true, async () => {
                    window.location.href = `topic-cards.html?subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&topicPosition=${nextTopic.position}&topicLevel=${nextTopic.level}&name=${encodeURIComponent(nextTopic.name)}&milestoneLevel=${milestoneLevel}`;
                });
            } else if (currentIndex + 1 < cards.length) {
                addNavButton('Next Card', true, async () => {
                    navigateToCard(cards[currentIndex + 1]);
                });
            }
        } else {
            // Section (Chapter) context
            const resourceId = parseInt(UI.getUrlParam('resourceId'));
            const sectionPosition = parseInt(UI.getUrlParam('sectionPosition'));
            const resourceName = UI.getUrlParam('resourceName') || '';
            const resourceType = UI.getUrlParam('resourceType') || '0';
            const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
            const resourceLink = UI.getUrlParam('resourceLink') || '';
            const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
            const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
            const subjectId = UI.getUrlParam('subjectId') || '';
            const subjectName = UI.getUrlParam('subjectName') || '';
            const roadmapId = UI.getUrlParam('roadmapId') || '';
            const roadmapName = UI.getUrlParam('roadmapName') || '';

            const sections = await client.getSections(resourceId);
            const sorted = sections.slice().sort((a, b) => a.position - b.position);
            const sIndex = sorted.findIndex(s => s.position === sectionPosition);

            // Previous: Show Previous Chapter only if at the first card of current section
            if (currentIndex === 0 && sIndex > 0) {
                const prevSection = sorted[sIndex - 1];
                addNavButton('Previous Chapter', false, async () => {
                    window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${prevSection.position}&name=${encodeURIComponent(prevSection.name)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink)}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
                });
            } else if (currentIndex > 0) {
                addNavButton('Previous Card', false, async () => {
                    navigateToCard(cards[currentIndex - 1]);
                });
            }

            // Next: Show Next Chapter only if at the last card of current section
            if (currentIndex === cards.length - 1 && sIndex + 1 < sorted.length) {
                const nextSection = sorted[sIndex + 1];
                addNavButton('Next Chapter', true, async () => {
                    window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${nextSection.position}&name=${encodeURIComponent(nextSection.name)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink)}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
                });
            } else if (currentIndex + 1 < cards.length) {
                addNavButton('Next Card', true, async () => {
                    navigateToCard(cards[currentIndex + 1]);
                });
            }
        }
    } catch (e) {
        console.error('Failed to build selective navigation:', e);
        // Fallback to minimal card navigation if topics/sections fetch fails
        if (currentIndex > 0) {
            addNavButton('Previous Card', false, async () => navigateToCard(cards[currentIndex - 1]));
        }
        if (currentIndex + 1 < cards.length) {
            addNavButton('Next Card', true, async () => navigateToCard(cards[currentIndex + 1]));
        }
    }

    navContainer.appendChild(infoText);
    navContainer.appendChild(buttonContainer);
    contentDiv.appendChild(navContainer);
}

function addPracticeNavigation(cardIndex, totalCards) {
    const contentDiv = document.getElementById('card-content');
    if (!contentDiv) return;

    const navContainer = document.createElement('div');
    navContainer.style.cssText = 'display: flex; justify-content: space-between; align-items: center; margin-top: 2rem; padding-top: 1rem; border-top: 2px solid var(--border-color);';

    const infoText = document.createElement('span');
    infoText.textContent = `Card ${cardIndex + 1} of ${totalCards}`;
    infoText.style.color = 'var(--text-muted)';

    const buttonContainer = document.createElement('div');
    buttonContainer.style.cssText = 'display: flex; gap: 0.5rem; flex-wrap: wrap; justify-content: flex-end;';

    // Check if there's a next card or topic
    const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');
    const hasNextCard = cardIndex + 1 < totalCards;
    const hasNextTopic = practiceState.topics && practiceState.currentTopicIndex < practiceState.topics.length - 1;

    // Check if there's a previous card or topic
    const hasPrevCard = cardIndex > 0;
    const hasPrevTopic = practiceState.topics && practiceState.currentTopicIndex > 0;

    // Previous buttons
    if (hasPrevTopic && cardIndex === 0) {
        const prevTopicBtn = document.createElement('button');
        prevTopicBtn.textContent = 'Previous Topic';
        prevTopicBtn.className = 'btn btn-secondary';
        prevTopicBtn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        prevTopicBtn.addEventListener('click', async () => {
            await recordProgressAndNavigate(async () => {
                await loadPreviousTopic();
            });
        });
        buttonContainer.appendChild(prevTopicBtn);
    } else if (hasPrevCard) {
        const prevBtn = document.createElement('button');
        prevBtn.textContent = 'Previous Card';
        prevBtn.className = 'btn btn-secondary';
        prevBtn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        prevBtn.addEventListener('click', async () => {
            await recordProgressAndNavigate(async () => {
                await loadPreviousCard(cardIndex, totalCards);
            });
        });
        buttonContainer.appendChild(prevBtn);
    }

    // Next buttons
    if (hasNextCard) {
        // Show "Next Card" button that records progress and navigates
        const nextBtn = document.createElement('button');
        nextBtn.textContent = 'Next Card';
        nextBtn.className = 'btn btn-primary';
        nextBtn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        nextBtn.addEventListener('click', async () => {
            await recordProgressAndNavigate(async () => {
                await loadNextCard(cardIndex, totalCards);
            });
        });
        buttonContainer.appendChild(nextBtn);
    } else if (hasNextTopic) {
        // Show "Next Topic" button that records progress and navigates
        const nextTopicBtn = document.createElement('button');
        nextTopicBtn.textContent = 'Next Topic';
        nextTopicBtn.className = 'btn btn-primary';
        nextTopicBtn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        nextTopicBtn.addEventListener('click', async () => {
            await recordProgressAndNavigate(async () => {
                await loadNextTopic();
            });
        });
        buttonContainer.appendChild(nextTopicBtn);
    } else {
        // Last card of last topic - show "Finish Practice" button
        const finishBtn = document.createElement('button');
        finishBtn.textContent = 'Finish Practice';
        finishBtn.id = 'record-progress-btn';
        finishBtn.className = 'btn btn-primary';
        finishBtn.style.cssText = 'padding: 0.5rem 1rem; white-space: nowrap;';
        finishBtn.addEventListener('click', async () => {
            await recordProgressAndNavigate(async () => {
                // Finish practice and go back to roadmap
                const roadmapId = UI.getUrlParam('roadmapId') || practiceState.roadmapId || '';
                const roadmapName = UI.getUrlParam('roadmapName') || '';
                sessionStorage.removeItem('practiceState');

                if (roadmapId && roadmapName) {
                    window.location.href = `roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}`;
                } else {
                    window.location.href = '/home.html';
                }
            });
        });
        buttonContainer.appendChild(finishBtn);
    }

    navContainer.appendChild(infoText);
    navContainer.appendChild(buttonContainer);
    contentDiv.appendChild(navContainer);
}

// Load previous card in current topic
async function loadPreviousCard(currentIndex, totalCards) {
    const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');
    if (!practiceState.topics || !practiceState.currentCards) {
        UI.showError('Practice state lost');
        return;
    }

    const prevIndex = currentIndex - 1;
    const prevCard = practiceState.currentCards[prevIndex];

    if (!prevCard) {
        UI.showError('No previous card found');
        return;
    }

    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicName = UI.getUrlParam('topicName') || '';
    const milestoneId = UI.getUrlParam('milestoneId') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
    window.location.href = `card.html?cardId=${prevCard.id}&headline=${encodeURIComponent(prevCard.headline)}&state=${prevCard.state}&cardIndex=${prevIndex}&totalCards=${totalCards}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(topicName)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
}

// Load previous topic
async function loadPreviousTopic() {
    const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');
    if (!practiceState.topics) {
        window.location.href = '/home.html';
        return;
    }

    const prevTopicIndex = practiceState.currentTopicIndex - 1;
    if (prevTopicIndex < 0) {
        return;
    }

    // Load previous topic
    const prevTopic = practiceState.topics[prevTopicIndex];

    try {
        const cards = await client.getPracticeCards(
            practiceState.roadmapId,
            practiceState.subjectId,
            prevTopic.level,
            prevTopic.position
        );

        if (cards.length === 0) {
            UI.showError('No cards in previous topic, skipping...');
            practiceState.currentTopicIndex = prevTopicIndex;
            sessionStorage.setItem('practiceState', JSON.stringify(practiceState));
            await loadPreviousTopic();
            return;
        }

        // Update practice state
        practiceState.currentTopicIndex = prevTopicIndex;
        practiceState.currentCards = cards;
        sessionStorage.setItem('practiceState', JSON.stringify(practiceState));

        // Navigate to last card of previous topic
        const roadmapId = UI.getUrlParam('roadmapId') || practiceState.roadmapId || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        const subjectId = UI.getUrlParam('subjectId') || practiceState.subjectId || '';
        const subjectName = UI.getUrlParam('subjectName') || '';
        const milestoneId = UI.getUrlParam('milestoneId') || '';
        const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
        const lastCardIndex = cards.length - 1;
        const lastCard = cards[lastCardIndex];
        
        window.location.href = `card.html?cardId=${lastCard.id}&headline=${encodeURIComponent(lastCard.headline)}&state=${lastCard.state}&cardIndex=${lastCardIndex}&totalCards=${cards.length}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(prevTopic.name)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
    } catch (err) {
        console.error('Failed to load previous topic:', err);
        UI.showError('Failed to load previous topic');
    }
}

// Load next card in current topic
async function loadNextCard(currentIndex, totalCards) {
    const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');
    if (!practiceState.topics || !practiceState.currentCards) {
        UI.showError('Practice state lost');
        return;
    }

    const nextIndex = currentIndex + 1;
    const nextCard = practiceState.currentCards[nextIndex];

    if (!nextCard) {
        UI.showError('No next card found');
        return;
    }

    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicName = UI.getUrlParam('topicName') || '';
    const milestoneId = UI.getUrlParam('milestoneId') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
    window.location.href = `card.html?cardId=${nextCard.id}&headline=${encodeURIComponent(nextCard.headline)}&state=${nextCard.state}&cardIndex=${nextIndex}&totalCards=${totalCards}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(topicName)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
}

// Load next topic or finish practice
async function loadNextTopic() {
    const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');
    if (!practiceState.topics) {
        window.location.href = '/home.html';
        return;
    }

    const nextTopicIndex = practiceState.currentTopicIndex + 1;

    if (nextTopicIndex >= practiceState.topics.length) {
        // All topics completed
        sessionStorage.removeItem('practiceState');
        alert('Practice session completed!');
        const currentTab = UI.getUrlParam('tab') || 'topics';
        window.location.href = `/subject.html?id=${practiceState.subjectId}&roadmapId=${practiceState.roadmapId}&level=${practiceState.milestoneLevel || ''}&tab=${currentTab}`;
        return;
    }

    // Load next topic
    const nextTopic = practiceState.topics[nextTopicIndex];

    try {
        const cards = await client.getPracticeCards(
            practiceState.roadmapId,
            practiceState.subjectId,
            nextTopic.level,
            nextTopic.position
        );

        if (cards.length === 0) {
            UI.showError('No cards in next topic, skipping...');
            practiceState.currentTopicIndex = nextTopicIndex;
            sessionStorage.setItem('practiceState', JSON.stringify(practiceState));
            await loadNextTopic();
            return;
        }

        // Update practice state
        practiceState.currentTopicIndex = nextTopicIndex;
        practiceState.currentCards = cards;
        sessionStorage.setItem('practiceState', JSON.stringify(practiceState));

        // Navigate to first card of next topic
        const roadmapId = UI.getUrlParam('roadmapId') || practiceState.roadmapId || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        const subjectId = UI.getUrlParam('subjectId') || practiceState.subjectId || '';
        const subjectName = UI.getUrlParam('subjectName') || '';
        const milestoneId = UI.getUrlParam('milestoneId') || '';
        const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&cardIndex=0&totalCards=${cards.length}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(nextTopic.name)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
    } catch (err) {
        console.error('Failed to load next topic:', err);
        UI.showError('Failed to load next topic');
    }
}

async function loadBlocks() {
    UI.toggleElement('loading', true);
    UI.toggleElement('card-content', false);
    UI.toggleElement('empty-state', false);

    try {
        const cardId = UI.getUrlParam('cardId');
        const blocks = await client.getBlocks(cardId);

        // Store blocks globally for editing
        currentBlocks = blocks;

        UI.toggleElement('loading', false);

        if (blocks.length === 0) {
            UI.toggleElement('card-content', true);
            UI.toggleElement('add-block-container', false);
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('card-content', true);
            UI.toggleElement('add-block-container', true);
            renderBlocks(blocks);
        }
    } catch (err) {
        console.error('Loading blocks failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load blocks: ' + (err.message || 'Unknown error'));
    }
}

// Map file extension to Prism language identifier
function mapExtensionToLanguage(extension) {
    if (!extension) return 'plaintext';

    const ext = extension.toLowerCase();
    const mapping = {
        'js': 'javascript',
        'ts': 'typescript',
        'jsx': 'jsx',
        'tsx': 'tsx',
        'py': 'python',
        'rb': 'ruby',
        'java': 'java',
        'c': 'c',
        'cpp': 'cpp',
        'cc': 'cpp',
        'cxx': 'cpp',
        'cs': 'csharp',
        'php': 'php',
        'swift': 'swift',
        'kt': 'kotlin',
        'go': 'go',
        'rs': 'rust',
        'scala': 'scala',
        'sh': 'bash',
        'bash': 'bash',
        'zsh': 'bash',
        'ps1': 'powershell',
        'sql': 'sql',
        'html': 'html',
        'htm': 'html',
        'xml': 'xml',
        'css': 'css',
        'scss': 'scss',
        'sass': 'sass',
        'less': 'less',
        'json': 'json',
        'yaml': 'yaml',
        'yml': 'yaml',
        'md': 'markdown',
        'markdown': 'markdown',
        'r': 'r',
        'lua': 'lua',
        'perl': 'perl',
        'dart': 'dart',
        'elixir': 'elixir',
        'clj': 'clojure',
        'clojure': 'clojure',
        'lisp': 'lisp',
        'vim': 'vim',
        'dockerfile': 'docker',
        'makefile': 'makefile',
        'graphql': 'graphql',
        'proto': 'protobuf',
        'toml': 'toml',
        'ini': 'ini',
        'tex': 'latex'
    };

    return mapping[ext] || ext;
}

function renderBlocks(blocks) {
    const container = document.getElementById('blocks-list');
    container.innerHTML = '';

    const blockTypes = ['text', 'code', 'image', 'math', 'diagram'];

    blocks.forEach((block, index) => {
        const blockItem = document.createElement('div');
        blockItem.className = 'content-block';
        blockItem.id = `block-${index}`;
        blockItem.draggable = true;
        blockItem.dataset.position = block.position;

        // Prevent dragging when user is selecting text or interacting with form elements
        blockItem.addEventListener('mousedown', (e) => {
            if (e.target.closest('textarea') ||
                e.target.closest('input') ||
                e.target.closest('select') ||
                e.target.closest('button') ||
                e.target.closest('.block-edit')) {
                blockItem.draggable = false;
            } else {
                blockItem.draggable = true;
            }
        });

        const typeName = blockTypes[block.type] || 'text';

        let metadataHtml = '';
        if (block.metadata) {
            metadataHtml = `<span class="block-metadata-badge">${UI.escapeHtml(block.metadata)}</span>`;
        }

        let contentHtml = '';
        if (block.type === 3) { // math
            try {
                if (typeof katex !== 'undefined') {
                    const html = katex.renderToString(block.content, {
                        throwOnError: false,
                        displayMode: true
                    });
                    contentHtml = `<div class="content-block-text math-content">${html}</div>`;
                } else {
                    contentHtml = `<div class="content-block-text" style="color: var(--color-error); font-style: italic;">Math renderer not available. Content: ${UI.escapeHtml(block.content)}</div>`;
                }
            } catch (err) {
                console.error('KaTeX rendering error:', err);
                contentHtml = `<div class="content-block-text" style="color: var(--color-error);">Math Error: ${UI.escapeHtml(err.message)}</div>`;
            }
        } else if (block.type === 1) { // code
            // Map extension to Prism language
            const language = mapExtensionToLanguage(block.extension);
            contentHtml = `<pre class="content-block-text" style="background: rgba(0, 0, 0, 0.3); padding: var(--space-md); border-radius: var(--radius-md); overflow-x: auto;"><code class="language-${language} show-language">${UI.escapeHtml(block.content)}</code></pre>`;
        } else if (block.type === 2) { // image
            contentHtml = `<img src="${UI.escapeHtml(block.content)}" alt="Block image" style="max-width: 100%; height: auto; border-radius: var(--radius-md);" />`;
        } else if (block.type === 4) { // diagram
            contentHtml = `<div id="diagram-${index}" class="content-block-diagram"></div>`;
            // Execute the JS code to render the diagram using Vega-Lite
            setTimeout(() => {
                const diagramContainer = document.getElementById(`diagram-${index}`);
                if (!diagramContainer) return;

                // Vega-Lite rendering
                if (typeof vegaEmbed !== 'undefined') {
                    try {
                        const spec = JSON.parse(block.content);
                        vegaEmbed(`#diagram-${index}`, spec, {
                            actions: false,
                            responsive: true,
                            width: "container"
                        }).catch(err => {
                            console.error('Vega-Lite rendering error:', err);
                            diagramContainer.innerHTML = `<div style="color: var(--color-error); padding: 1rem; border: 1px solid var(--color-error); border-radius: 4px;">Vega Error: ${UI.escapeHtml(err.message)}</div>`;
                        });
                    } catch (err) {
                        console.error('JSON parse error for Vega-Lite:', err);
                        diagramContainer.innerHTML = `<div style="color: var(--color-error); padding: 1rem; border: 1px solid var(--color-error); border-radius: 4px;">JSON Error: ${UI.escapeHtml(err.message)}</div>`;
                    }
                } else {
                    diagramContainer.innerHTML = `<div style="color: var(--color-error); padding: 1rem;">Vega-Lite library not loaded.</div>`;
                }
            }, 0);
        } else { // text (type 0 or default)
            // Parse markdown for md or txt extensions
            const ext = block.extension ? block.extension.toLowerCase() : '';
            if ((ext === 'md' || ext === 'txt') && typeof marked !== 'undefined') {
                // Configure marked for better rendering
                marked.setOptions({
                    breaks: true,
                    gfm: true
                });
                contentHtml = `<div class="content-block-text markdown-content">${marked.parse(block.content)}</div>`;
            } else {
                contentHtml = `<div class="content-block-text">${UI.escapeHtml(block.content)}</div>`;
            }
        }

        // Display mode
        const displayDiv = document.createElement('div');
        displayDiv.className = 'block-display';
        displayDiv.id = `block-display-${index}`;

        displayDiv.innerHTML = `
            ${metadataHtml}
            <div class="block-actions-overlay">
                <button class="block-action-btn block-edit-btn" onclick="editBlock(${index})">
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"></path>
                        <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"></path>
                    </svg>
                    Edit
                </button>
                <button class="block-action-btn block-remove-btn" data-position="${block.position}" data-index="${index}">
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <polyline points="3 6 5 6 21 6"></polyline>
                        <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                    </svg>
                    Remove
                </button>
            </div>
            ${contentHtml}
        `;

        // Show actions on tap for mobile
        blockItem.addEventListener('click', (e) => {
            // Only toggle on touch devices (where we don't have hover)
            // or if we want it to work on click too.
            // But let's check if it's not a button click itself.
            if (e.target.closest('.block-action-btn')) return;

            // Don't toggle actions if inside edit form
            if (e.target.closest('.block-edit')) return;

            const isShowing = blockItem.classList.contains('show-actions');

            // Remove from others first
            document.querySelectorAll('.content-block.show-actions').forEach(el => {
                el.classList.remove('show-actions');
            });
            
            if (!isShowing) {
                blockItem.classList.add('show-actions');
            }
            e.stopPropagation(); // Prevent immediate close from document mousedown
        });

        blockItem.addEventListener('dragstart', (e) => {
            // Don't allow dragging if user is interacting with form elements
            if (e.target.closest('textarea') ||
                e.target.closest('input') ||
                e.target.closest('select') ||
                e.target.closest('.block-edit')) {
                e.preventDefault();
                return;
            }

            isDragging = true;
            blockItem.classList.add('dragging');
            e.dataTransfer.effectAllowed = 'move';
            e.dataTransfer.setData('text/plain', JSON.stringify({
                position: block.position
            }));
        });

        blockItem.addEventListener('dragend', () => {
            blockItem.classList.remove('dragging');
            setTimeout(() => {
                isDragging = false;
            }, 100);
        });

        blockItem.addEventListener('dragover', (e) => {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'move';

            const draggingBlock = document.querySelector('.dragging');
            if (draggingBlock && draggingBlock !== blockItem) {
                blockItem.classList.add('drag-over');
            }
        });

        blockItem.addEventListener('dragleave', () => {
            blockItem.classList.remove('drag-over');
        });

        blockItem.addEventListener('drop', async (e) => {
            e.preventDefault();
            blockItem.classList.remove('drag-over');

            const dragData = JSON.parse(e.dataTransfer.getData('text/plain') || '{}');
            const sourcePosition = dragData.position;
            const targetPosition = block.position;

            if (sourcePosition !== targetPosition) {
                await reorderBlockHandler(sourcePosition, targetPosition);
            }
        });

        // Touch event handlers for mobile drag-and-drop
        let touchStartY = 0;
        let touchStartX = 0;
        let touchCurrentY = 0;
        let touchStartElement = null;
        let touchClone = null;
        let touchTargetElement = null;
        let longPressTimer = null;
        let isDragEnabled = false;

        blockItem.addEventListener('touchstart', (e) => {
            // Don't interfere with buttons, inputs, textareas, or any edit form elements
            if (e.target.closest('button') ||
                e.target.closest('input') ||
                e.target.closest('textarea') ||
                e.target.closest('select') ||
                e.target.closest('.block-edit') ||
                e.target.tagName === 'TEXTAREA' ||
                e.target.tagName === 'INPUT' ||
                e.target.tagName === 'SELECT') {
                return;
            }

            touchStartY = e.touches[0].clientY;
            touchStartX = e.touches[0].clientX;
            touchCurrentY = touchStartY;
            touchStartElement = blockItem;
            isDragEnabled = false;

            // Start long-press timer (500ms)
            longPressTimer = setTimeout(() => {
                isDragEnabled = true;

                // Vibrate for feedback if available
                if (navigator.vibrate) {
                    navigator.vibrate(50);
                }

                // Create a visual clone for dragging
                touchClone = blockItem.cloneNode(true);
                touchClone.style.position = 'fixed';
                touchClone.style.top = blockItem.getBoundingClientRect().top + 'px';
                touchClone.style.left = blockItem.getBoundingClientRect().left + 'px';
                touchClone.style.width = blockItem.offsetWidth + 'px';
                touchClone.style.opacity = '0.8';
                touchClone.style.pointerEvents = 'none';
                touchClone.style.zIndex = '1000';
                touchClone.classList.add('dragging');
                document.body.appendChild(touchClone);

                blockItem.style.opacity = '0.3';
            }, 500);
        }, { passive: true });

        blockItem.addEventListener('touchmove', (e) => {
            if (!touchStartElement) return;

            // Don't interfere with text selection in form elements
            if (e.target.closest('textarea') ||
                e.target.closest('input') ||
                e.target.closest('select') ||
                e.target.closest('.block-edit')) {
                return;
            }

            // Calculate movement distance
            const deltaX = Math.abs(e.touches[0].clientX - touchStartX);
            const deltaY = Math.abs(e.touches[0].clientY - touchStartY);

            // Issue #1 fix: If user moved even slightly before long-press completed, cancel it (they're scrolling)
            // Reduced threshold to 5px to be more sensitive to scrolling
            if (!isDragEnabled && (deltaX > 5 || deltaY > 5)) {
                if (longPressTimer) {
                    clearTimeout(longPressTimer);
                    longPressTimer = null;
                }
                touchStartElement = null;
                return;
            }

            // Only proceed if drag is enabled
            if (!isDragEnabled || !touchClone) return;

            e.preventDefault(); // Prevent scrolling while dragging
            touchCurrentY = e.touches[0].clientY;
            const dragDeltaY = touchCurrentY - touchStartY;

            // Move the clone
            const rect = touchStartElement.getBoundingClientRect();
            touchClone.style.top = (rect.top + dragDeltaY) + 'px';

            // Find the element under the touch point
            touchClone.style.display = 'none';
            const elementBelow = document.elementFromPoint(e.touches[0].clientX, e.touches[0].clientY);
            touchClone.style.display = 'block';

            const blockBelow = elementBelow ? elementBelow.closest('.content-block') : null;

            // Remove drag-over class from all blocks
            document.querySelectorAll('.content-block').forEach(b => b.classList.remove('drag-over'));

            if (blockBelow && blockBelow !== touchStartElement) {
                touchTargetElement = blockBelow;
                blockBelow.classList.add('drag-over');
            } else {
                touchTargetElement = null;
            }
        }, { passive: false });

        blockItem.addEventListener('touchend', async (e) => {
            // Clear the long-press timer if it hasn't fired yet
            if (longPressTimer) {
                clearTimeout(longPressTimer);
                longPressTimer = null;
            }

            // Only proceed if drag was enabled
            if (!isDragEnabled) {
                touchStartElement = null;
                return;
            }

            if (!touchStartElement || !touchClone) return;

            e.preventDefault();

            // Remove the clone
            if (touchClone && touchClone.parentNode) {
                touchClone.parentNode.removeChild(touchClone);
            }

            // Restore opacity
            touchStartElement.style.opacity = '1';

            // Remove drag-over class from all blocks
            document.querySelectorAll('.content-block').forEach(b => b.classList.remove('drag-over'));

            // Perform the reorder if dropped on a different block
            if (touchTargetElement && touchTargetElement !== touchStartElement) {
                const sourcePosition = parseInt(touchStartElement.dataset.position);
                const targetPosition = parseInt(touchTargetElement.dataset.position);

                if (!isNaN(sourcePosition) && !isNaN(targetPosition) && sourcePosition !== targetPosition) {
                    await reorderBlockHandler(sourcePosition, targetPosition);
                }
            }

            // Reset touch state
            touchStartY = 0;
            touchStartX = 0;
            touchCurrentY = 0;
            touchStartElement = null;
            touchClone = null;
            touchTargetElement = null;
            isDragEnabled = false;
        });

        blockItem.addEventListener('touchcancel', () => {
            // Clear the long-press timer
            if (longPressTimer) {
                clearTimeout(longPressTimer);
                longPressTimer = null;
            }

            if (touchClone && touchClone.parentNode) {
                touchClone.parentNode.removeChild(touchClone);
            }
            if (touchStartElement) {
                touchStartElement.style.opacity = '1';
            }
            document.querySelectorAll('.content-block').forEach(b => b.classList.remove('drag-over'));
            touchStartY = 0;
            touchStartX = 0;
            touchCurrentY = 0;
            touchStartElement = null;
            touchClone = null;
            touchTargetElement = null;
            isDragEnabled = false;
        });


        // Remove button handler
        const removeBtn = displayDiv.querySelector('.block-remove-btn');
        if (removeBtn) {
            removeBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                showRemoveBlockModal(block.position);
            });
        }

        // Edit mode
        const editDiv = document.createElement('div');
        editDiv.className = 'block-edit';
        editDiv.id = `block-edit-${index}`;
        editDiv.style.display = 'none';

        const extensionValue = block.extension !== undefined && block.extension !== null ? block.extension : '';
        const metadataValue = block.metadata !== undefined && block.metadata !== null ? block.metadata : '';
        const contentValue = block.content !== undefined && block.content !== null ? block.content : '';

        editDiv.innerHTML = `
            <div style="margin-bottom: 1rem;">
                <label style="display: block; margin-bottom: 0.25rem; font-weight: 500;">Type: <span style="color: red;">*</span></label>
                <select id="block-type-${index}" class="form-input" style="width: 100%;">
                    <option value="0" ${block.type === 0 ? 'selected' : ''}>Text</option>
                    <option value="1" ${block.type === 1 ? 'selected' : ''}>Code</option>
                    <option value="2" ${block.type === 2 ? 'selected' : ''}>Image</option>
                    <option value="3" ${block.type === 3 ? 'selected' : ''}>Math</option>
                    <option value="4" ${block.type === 4 ? 'selected' : ''}>Diagram</option>
                </select>
            </div>
            <div style="margin-bottom: 1rem;">
                <label style="display: block; margin-bottom: 0.25rem; font-weight: 500;">Extension: <span style="color: red;">*</span></label>
                <input type="text" id="block-extension-${index}" class="form-input" style="width: 100%;" value="${UI.escapeHtml(extensionValue)}" placeholder="e.g., js, py, cpp" required>
            </div>
            <div style="margin-bottom: 1rem;">
                <label style="display: block; margin-bottom: 0.25rem; font-weight: 500;">Metadata (optional):</label>
                <input type="text" id="block-metadata-${index}" class="form-input" style="width: 100%;" value="${UI.escapeHtml(metadataValue)}" placeholder="e.g., Important, Note, Warning">
            </div>
            <div style="margin-bottom: 1rem;">
                <label style="display: block; margin-bottom: 0.25rem; font-weight: 500;">Content: <span style="color: red;">*</span></label>
                <p style="font-size: 0.875rem; color: var(--text-muted); margin-bottom: 0.5rem;">
                    💡 Tip: Leave a blank line (press Enter twice) to split this block into multiple blocks
                </p>
                <textarea id="block-content-${index}" class="form-input" style="width: 100%; min-height: 150px; font-family: monospace;" placeholder="Enter block content..." required>${UI.escapeHtml(contentValue)}</textarea>
            </div>
            <div style="display: flex; gap: 0.5rem; align-items: center;">
                <button class="btn btn-primary" onclick="saveBlock(${index})" id="save-block-btn-${index}">Save</button>
                <button class="btn btn-secondary" onclick="cancelEditBlock(${index})">Cancel</button>
                <button class="btn btn-secondary" onclick="saveSplitBlock(${index})" id="split-block-btn-${index}" style="display: none; background-color: #d4845c; color: white;">Save & Split</button>
            </div>
        `;

        blockItem.appendChild(displayDiv);
        blockItem.appendChild(editDiv);
        container.appendChild(blockItem);

        // Add auto-split detection for content textarea
        // Wait for next tick to ensure textarea is in DOM
        setTimeout(() => {
            const contentTextarea = document.getElementById(`block-content-${index}`);
            const splitBtn = document.getElementById(`split-block-btn-${index}`);
            const typeSelect = document.getElementById(`block-type-${index}`);
            const extensionInput = document.getElementById(`block-extension-${index}`);

            if (typeSelect && extensionInput) {
                const handleTypeChange = () => {
                    const type = parseInt(typeSelect.value);
                    if (type === 0) { // Text
                        extensionInput.value = 'md';
                        extensionInput.disabled = true;
                    } else if (type === 3) { // Math
                        extensionInput.value = 'tex';
                        extensionInput.disabled = true;
                    } else if (type === 4) { // Diagram
                        extensionInput.value = 'json';
                        extensionInput.disabled = true;
                    } else {
                        extensionInput.disabled = false;
                    }
                };

                typeSelect.addEventListener('change', handleTypeChange);
                // Set initial state
                handleTypeChange();

                // Ensure extension is always lower case as user types
                extensionInput.addEventListener('input', () => {
                    extensionInput.value = extensionInput.value.toLowerCase();
                });
            }

            if (contentTextarea && splitBtn) {
                contentTextarea.addEventListener('input', () => {
                    const content = contentTextarea.value;
                    // Check if content contains three newlines (blank line between content)
                    if (content.includes('\n\n\n')) {
                        splitBtn.style.display = 'inline-block';
                    } else {
                        splitBtn.style.display = 'none';
                    }
                });

                // Issue #3 fix: Mark form elements so block handlers can ignore them
                // Don't stop propagation - let native browser behavior work
                // The block's touchstart handler already checks for .block-edit

                // No special handling needed - the block's handlers already exclude .block-edit
                // Native browser input handling will work correctly
            }
        }, 0);
    });

    // Apply KaTeX auto-render to all text blocks
    if (typeof renderMathInElement !== 'undefined') {
        renderMathInElement(container, {
            delimiters: [
                {left: '$$', right: '$$', display: true},
                {left: '$', right: '$', display: false}
            ],
            throwOnError: false
        });
    }

    // Apply Prism syntax highlighting to all code blocks
    if (typeof Prism !== 'undefined') {
        Prism.highlightAll();
    }
}

// Global variable to store current blocks
let currentBlocks = [];

window.editBlock = function(index) {
    const displayDiv = document.getElementById(`block-display-${index}`);
    const editDiv = document.getElementById(`block-edit-${index}`);
    const blockItem = document.getElementById(`block-${index}`);

    if (displayDiv && editDiv) {
        displayDiv.style.display = 'none';
        editDiv.style.display = 'block';

        // Disable dragging when in edit mode
        if (blockItem) {
            blockItem.draggable = false;
            blockItem.style.cursor = 'default';
        }
    }
};

window.cancelEditBlock = function(index) {
    const displayDiv = document.getElementById(`block-display-${index}`);
    const editDiv = document.getElementById(`block-edit-${index}`);
    const blockItem = document.getElementById(`block-${index}`);

    if (displayDiv && editDiv) {
        displayDiv.style.display = 'block';
        editDiv.style.display = 'none';

        // Re-enable dragging when exiting edit mode
        if (blockItem) {
            blockItem.draggable = true;
            blockItem.style.cursor = 'grab';
        }
    }
};

window.saveBlock = async function(index) {
    const block = currentBlocks[index];
    if (!block) {
        UI.showError('Block not found');
        return;
    }

    const typeSelect = document.getElementById(`block-type-${index}`);
    const extensionInput = document.getElementById(`block-extension-${index}`);
    const metadataInput = document.getElementById(`block-metadata-${index}`);
    const contentTextarea = document.getElementById(`block-content-${index}`);

    if (!typeSelect || !extensionInput || !metadataInput || !contentTextarea) {
        UI.showError('Form elements not found');
        return;
    }

    const newType = parseInt(typeSelect.value);
    const newExtension = extensionInput.value.trim().toLowerCase();
    const newMetadata = metadataInput.value.trim();
    const newContent = contentTextarea.value.trim();

    if (!newContent) {
        UI.showError('Content cannot be empty');
        return;
    }

    if (!newExtension) {
        UI.showError('Extension cannot be empty');
        return;
    }

    if (isNaN(newType)) {
        UI.showError('Type must be selected');
        return;
    }

    UI.setButtonLoading(`save-block-btn-${index}`, true);

    try {
        const cardId = parseInt(UI.getUrlParam('cardId'));

        await client.editBlock(cardId, block.position, newType, newExtension, newContent, newMetadata);

        // Update the block in our current blocks array
        currentBlocks[index] = {
            ...block,
            type: newType,
            extension: newExtension,
            metadata: newMetadata,
            content: newContent
        };

        // Re-render all blocks to show the updated content
        renderBlocks(currentBlocks);

        UI.setButtonLoading(`save-block-btn-${index}`, false);
    } catch (err) {
        console.error('Failed to edit block:', err);

        // Error code 6 means ALREADY_EXISTS (no changes detected)
        if (err.code === 6) {
            // No changes, just exit edit mode
            const displayDiv = document.getElementById(`block-display-${index}`);
            const editDiv = document.getElementById(`block-edit-${index}`);
            if (displayDiv && editDiv) {
                displayDiv.style.display = 'block';
                editDiv.style.display = 'none';
            }
        } else {
            UI.showError('Failed to edit block: ' + (err.message || 'Unknown error'));
        }

        UI.setButtonLoading(`save-block-btn-${index}`, false);
    }
};

window.saveSplitBlock = async function(index) {
    const block = currentBlocks[index];
    if (!block) {
        UI.showError('Block not found');
        return;
    }

    const typeSelect = document.getElementById(`block-type-${index}`);
    const extensionInput = document.getElementById(`block-extension-${index}`);
    const metadataInput = document.getElementById(`block-metadata-${index}`);
    const contentTextarea = document.getElementById(`block-content-${index}`);

    if (!typeSelect || !extensionInput || !metadataInput || !contentTextarea) {
        UI.showError('Form elements not found');
        return;
    }

    const newType = parseInt(typeSelect.value);
    const newExtension = extensionInput.value.trim().toLowerCase();
    const newMetadata = metadataInput.value.trim();
    const newContent = contentTextarea.value.trim();

    if (!newContent) {
        UI.showError('Content cannot be empty');
        return;
    }

    if (!newExtension) {
        UI.showError('Extension cannot be empty');
        return;
    }

    if (isNaN(newType)) {
        UI.showError('Type must be selected');
        return;
    }

    // Check if content actually contains three newlines (blank line)
    if (!newContent.includes('\n\n\n')) {
        UI.showError('Content must contain a blank line (three newlines) to split');
        return;
    }

    UI.setButtonLoading(`split-block-btn-${index}`, true);

    try {
        const cardId = parseInt(UI.getUrlParam('cardId'));

        // First, update the block with the new content
        await client.editBlock(cardId, block.position, newType, newExtension, newContent, newMetadata);

        // Then, split the block
        await client.splitBlock(cardId, block.position);

        // Reload all blocks to show the split result
        await loadBlocks();

        UI.showSuccess('Block split successfully');
        UI.setButtonLoading(`split-block-btn-${index}`, false);
    } catch (err) {
        console.error('Failed to split block:', err);
        UI.showError('Failed to split block: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading(`split-block-btn-${index}`, false);
    }
};


function openRemoveCardModal() {
    const modal = document.getElementById('remove-card-modal');
    if (modal) {
        modal.style.display = 'flex';
        document.body.style.overflow = 'hidden';
    }
}

function closeRemoveCardModal() {
    const modal = document.getElementById('remove-card-modal');
    if (modal) {
        modal.style.display = 'none';
        document.body.style.overflow = '';
    }
}

async function confirmRemoveCard() {
    const cardId = parseInt(UI.getUrlParam('cardId'));
    const resourceId = UI.getUrlParam('resourceId');
    const sectionPosition = UI.getUrlParam('sectionPosition');
    const topicPosition = UI.getUrlParam('topicPosition');
    const topicLevel = UI.getUrlParam('topicLevel');
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicName = UI.getUrlParam('topicName') || '';
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';

    if (!cardId) {
        UI.showError('Invalid card ID');
        return;
    }

    UI.setButtonLoading('confirm-remove-card-btn', true);

    try {
        await client.removeCard(cardId);

        // Navigate back to the appropriate page
        if (resourceId && sectionPosition !== '') {
            // Navigate back to section cards page
            const resourceName = UI.getUrlParam('resourceName') || '';
            const sectionName = UI.getUrlParam('sectionName') || '';
            const resourceType = UI.getUrlParam('resourceType') || '0';
            const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
            const resourceLink = UI.getUrlParam('resourceLink') || '';
            const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
            const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';

            window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${sectionPosition}&name=${encodeURIComponent(sectionName)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink)}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
        } else if (topicPosition !== '' && topicLevel !== '') {
            // Navigate back to topic cards page
            window.location.href = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&name=${encodeURIComponent(topicName)}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
        } else if (subjectId) {
            // If accessed directly or from subject practice, go back to subject page
            const currentTab = UI.getUrlParam('tab') || (topicPosition !== '' && topicLevel !== '' ? 'topics' : (resourceId && sectionPosition !== '' ? 'resources' : 'topics'));
            window.location.href = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
        } else {
            // Fallback to home
            window.location.href = '/home.html';
        }
    } catch (err) {
        console.error('Failed to remove card:', err);
        UI.showError('Failed to remove card: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('confirm-remove-card-btn', false);
        closeRemoveCardModal();
    }
}

// Show remove block modal
function showRemoveBlockModal(position) {
    const modal = document.getElementById('remove-block-modal');

    if (modal) {
        UI.toggleElement('remove-block-modal', true);
        document.body.style.overflow = 'hidden';

        // Store position in modal for use when confirming
        modal.dataset.blockPosition = position;
    }
}

// Close remove block modal
function closeRemoveBlockModal() {
    const modal = document.getElementById('remove-block-modal');

    if (modal) {
        UI.toggleElement('remove-block-modal', false);
        document.body.style.overflow = '';
        delete modal.dataset.blockPosition;
    }
}

// Remove block handler
async function removeBlockHandler(position) {
    const cardId = parseInt(UI.getUrlParam('cardId'));

    UI.setButtonLoading('confirm-remove-block-btn', true);

    try {
        await client.removeBlock(cardId, position);
        closeRemoveBlockModal();
        UI.setButtonLoading('confirm-remove-block-btn', false);
        await loadBlocks();
        UI.showSuccess('Block removed successfully');
    } catch (err) {
        console.error('Remove block failed:', err);
        UI.showError('Failed to remove block: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('confirm-remove-block-btn', false);
    }
}

// Reorder block handler
async function reorderBlockHandler(sourcePosition, targetPosition) {
    const cardId = parseInt(UI.getUrlParam('cardId'));

    try {
        await client.reorderBlock(cardId, sourcePosition, targetPosition);
        await loadBlocks();
    } catch (err) {
        console.error('Reorder block failed:', err);
        UI.showError('Failed to reorder block: ' + (err.message || 'Unknown error'));
    }
}
