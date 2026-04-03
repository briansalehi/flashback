async function markSectionAsReviewed() {
    const resourceId = parseInt(UI.getUrlParam('resourceId'));
    const sectionPosition = parseInt(UI.getUrlParam('sectionPosition'));
    const markSectionReviewedBtn = document.getElementById('mark-section-reviewed-btn');

    if (!resourceId || isNaN(sectionPosition)) {
        UI.showError('Invalid resource ID or section position');
        return;
    }

    UI.setButtonLoading('mark-section-reviewed-btn', true);

    try {
        await client.markSectionAsReviewed(resourceId, sectionPosition);

        // Update state display
        const stateBadge = document.getElementById('section-state-badge');
        if (stateBadge) {
            stateBadge.textContent = 'reviewed';
            stateBadge.className = 'state-badge reviewed';
        }

        // Update URL state parameter to 1 (reviewed)
        const url = new URL(window.location.href);
        url.searchParams.set('sectionState', '1');
        window.history.replaceState({}, '', url);

        // Update the button for the next state (Completed)
        if (markSectionReviewedBtn) {
            markSectionReviewedBtn.title = 'Mark as Completed';
            markSectionReviewedBtn.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-check-check"><path d="m4 9 5 5 11-11"></path><path d="m4 15 5 5 11-11"></path></svg>';
        }

        // Re-render cards to update their links with the new section state
        const cards = await client.getSectionCards(resourceId, sectionPosition);
        renderCards(cards);

        UI.setButtonLoading('mark-section-reviewed-btn', false);
    } catch (err) {
        console.error('Failed to mark section as reviewed:', err);
        UI.showError('Failed to mark section as reviewed: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('mark-section-reviewed-btn', false);
    }
}

async function markSectionAsCompleted() {
    const resourceId = parseInt(UI.getUrlParam('resourceId'));
    const sectionPosition = parseInt(UI.getUrlParam('sectionPosition'));
    const markSectionReviewedBtn = document.getElementById('mark-section-reviewed-btn');

    if (!resourceId || isNaN(sectionPosition)) {
        UI.showError('Invalid resource ID or section position');
        return;
    }

    UI.setButtonLoading('mark-section-reviewed-btn', true);

    try {
        const cards = await client.getSectionCards(resourceId, sectionPosition);
        const draftCards = cards.filter(card => card.state === 0);
        if (draftCards.length > 0) {
            UI.showError('All cards in this section must be reviewed before marking the section as completed.');
            UI.setButtonLoading('mark-section-reviewed-btn', false);
            return;
        }

        await client.markSectionAsCompleted(resourceId, sectionPosition);

        // Update state display
        const stateBadge = document.getElementById('section-state-badge');
        if (stateBadge) {
            stateBadge.textContent = 'completed';
            stateBadge.className = 'state-badge completed';
        }

        // Update URL state parameter to 2 (completed)
        const url = new URL(window.location.href);
        url.searchParams.set('sectionState', '2');
        window.history.replaceState({}, '', url);

        // Hide the button as it's now completed
        if (markSectionReviewedBtn) {
            markSectionReviewedBtn.style.display = 'none';
        }

        // Re-render cards to update their links with the new section state
        renderCards(cards);

        UI.setButtonLoading('mark-section-reviewed-btn', false);
    } catch (err) {
        console.error('Failed to mark section as completed:', err);
        UI.showError('Failed to mark section as completed: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('mark-section-reviewed-btn', false);
    }
}

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const resourceId = parseInt(UI.getUrlParam('resourceId'));
    const sectionPosition = parseInt(UI.getUrlParam('sectionPosition'));
    const sectionState = parseInt(UI.getUrlParam('sectionState')) || 0;
    const sectionName = UI.getUrlParam('name');
    currentSectionName = sectionName;
    const resourceName = UI.getUrlParam('resourceName');

    if (isNaN(resourceId) || isNaN(sectionPosition)) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('section-name').textContent = sectionName || 'Section';
    document.title = `${sectionName || 'Section'} - Flashback`;

    // Initialize button visibility
    const editBtn = document.getElementById('edit-section-btn');
    const removeBtn = document.getElementById('remove-section-btn');
    const addCardBtn = document.getElementById('add-card-btn');
    const markReviewedBtn = document.getElementById('mark-section-reviewed-btn');

    if (editBtn) editBtn.style.display = 'inline-flex';
    if (removeBtn) removeBtn.style.display = 'inline-flex';
    if (addCardBtn) {
        addCardBtn.style.display = 'flex';
        addCardBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-card-modal', true);
            document.body.style.overflow = 'hidden';
            setTimeout(() => {
                const headlineInput = document.getElementById('card-headline');
                if (headlineInput) {
                    headlineInput.focus();
                }
            }, 100);
        });
    }
    if (markReviewedBtn && sectionState < 2) {
        markReviewedBtn.style.display = 'inline-flex';
        if (sectionState === 1) {
            markReviewedBtn.title = 'Mark as Completed';
            markReviewedBtn.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lucide lucide-check-check"><path d="m4 9 5 5 11-11"></path><path d="m4 15 5 5 11-11"></path></svg>';
        }
    }

    // Display breadcrumb
    displayBreadcrumb();

    // Display section state badge
    const stateNames = ['draft', 'reviewed', 'completed'];
    const stateName = stateNames[sectionState] || 'draft';
    const stateBadge = document.getElementById('section-state-badge');
    if (stateBadge) {
        stateBadge.textContent = stateName;
        stateBadge.className = `state-badge ${stateName}`;
    }

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    const closeCardModalBtn = document.getElementById('close-card-modal-btn');
    if (closeCardModalBtn) {
        closeCardModalBtn.addEventListener('click', () => {
            UI.toggleElement('add-card-modal', false);
            document.body.style.overflow = 'auto';
            UI.clearForm('card-form');
        });
    }

    const cancelCardBtn = document.getElementById('cancel-card-btn');
    if (cancelCardBtn) {
        cancelCardBtn.addEventListener('click', () => {
            UI.toggleElement('add-card-modal', false);
            document.body.style.overflow = 'auto';
            UI.clearForm('card-form');
        });
    }

    const addCardModal = document.getElementById('add-card-modal');
    if (addCardModal) {
        addCardModal.addEventListener('click', (e) => {
            if (e.target === addCardModal) {
                UI.toggleElement('add-card-modal', false);
                document.body.style.overflow = 'auto';
                UI.clearForm('card-form');
            }
        });
    }

    const cardForm = document.getElementById('card-form');
    if (cardForm) {
        cardForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const headline = document.getElementById('card-headline').value.trim();

            if (!headline) {
                UI.showError('Please enter a card headline');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-card-btn', true);

            try {
                const card = await client.createCard(headline);
                await client.addCardToSection(card.id, resourceId, sectionPosition);

                newlyCreatedCardId = card.id;

                UI.toggleElement('add-card-modal', false);
                document.body.style.overflow = 'auto';
                UI.clearForm('card-form');
                UI.setButtonLoading('save-card-btn', false);

                loadCards();
            } catch (err) {
                console.error('Add card failed:', err);
                UI.showError(err.message || 'Failed to add card');
                UI.setButtonLoading('save-card-btn', false);
            }
        });
    }

    // Setup mark section as reviewed button
    const markSectionReviewedBtn = document.getElementById('mark-section-reviewed-btn');
    if (markSectionReviewedBtn) {
        const openReviewSectionModal = () => {
            const modal = document.getElementById('review-section-confirmation-modal');
            if (modal) {
                modal.style.display = 'flex';
                document.body.style.overflow = 'hidden';
            }
        };

        const closeReviewSectionModal = () => {
            const modal = document.getElementById('review-section-confirmation-modal');
            if (modal) {
                modal.style.display = 'none';
                document.body.style.overflow = '';
            }
        };

        const openCompleteSectionModal = () => {
            const modal = document.getElementById('complete-section-confirmation-modal');
            if (modal) {
                modal.style.display = 'flex';
                document.body.style.overflow = 'hidden';
            }
        };

        const closeCompleteSectionModal = () => {
            const modal = document.getElementById('complete-section-confirmation-modal');
            if (modal) {
                modal.style.display = 'none';
                document.body.style.overflow = '';
            }
        };

        markSectionReviewedBtn.addEventListener('click', (e) => {
            e.preventDefault();
            const sectionState = parseInt(UI.getUrlParam('sectionState')) || 0;
            if (sectionState === 1) {
                openCompleteSectionModal();
            } else {
                openReviewSectionModal();
            }
        });

        const cancelReviewSectionBtn = document.getElementById('cancel-review-section-btn');
        if (cancelReviewSectionBtn) cancelReviewSectionBtn.addEventListener('click', closeReviewSectionModal);

        const closeReviewSectionModalBtn = document.getElementById('close-review-section-modal-btn');
        if (closeReviewSectionModalBtn) closeReviewSectionModalBtn.addEventListener('click', closeReviewSectionModal);

        const reviewSectionModal = document.getElementById('review-section-confirmation-modal');
        if (reviewSectionModal) {
            reviewSectionModal.addEventListener('click', (e) => {
                if (e.target === reviewSectionModal) closeReviewSectionModal();
            });
        }

        const confirmReviewSectionBtn = document.getElementById('confirm-review-section-btn');
        if (confirmReviewSectionBtn) {
            confirmReviewSectionBtn.addEventListener('click', async () => {
                await markSectionAsReviewed();
                closeReviewSectionModal();
            });
        }

        // Completed section modal handlers
        const cancelCompleteSectionBtn = document.getElementById('cancel-complete-section-btn');
        if (cancelCompleteSectionBtn) cancelCompleteSectionBtn.addEventListener('click', closeCompleteSectionModal);

        const closeCompleteSectionModalBtn = document.getElementById('close-complete-section-modal-btn');
        if (closeCompleteSectionModalBtn) closeCompleteSectionModalBtn.addEventListener('click', closeCompleteSectionModal);

        const completeSectionModal = document.getElementById('complete-section-confirmation-modal');
        if (completeSectionModal) {
            completeSectionModal.addEventListener('click', (e) => {
                if (e.target === completeSectionModal) closeCompleteSectionModal();
            });
        }

        const confirmCompleteSectionBtn = document.getElementById('confirm-complete-section-btn');
        if (confirmCompleteSectionBtn) {
            confirmCompleteSectionBtn.addEventListener('click', async () => {
                await markSectionAsCompleted();
                closeCompleteSectionModal();
            });
        }
    }

    // Edit section handlers
    const editSectionBtn = document.getElementById('edit-section-btn');
    const editSectionModal = document.getElementById('edit-section-modal');
    const closeEditSectionModalBtn = document.getElementById('close-edit-section-modal-btn');
    const cancelEditSectionBtn = document.getElementById('cancel-edit-section-btn');

    const closeEditSection = () => {
        UI.toggleElement('edit-section-modal', false);
        document.body.style.overflow = '';
        UI.clearForm('edit-section-form');
    };

    if (editSectionBtn) {
        editSectionBtn.addEventListener('click', () => {
            UI.toggleElement('edit-section-modal', true);
            document.body.style.overflow = 'hidden';
            document.getElementById('edit-section-name').value = sectionName || '';
            const sectionLink = UI.getUrlParam('sectionLink') || '';
            document.getElementById('edit-section-link').value = sectionLink;
            setTimeout(() => {
                document.getElementById('edit-section-name').focus();
            }, 100);
        });
    }
    if (closeEditSectionModalBtn) closeEditSectionModalBtn.addEventListener('click', closeEditSection);
    if (cancelEditSectionBtn) cancelEditSectionBtn.addEventListener('click', closeEditSection);
    if (editSectionModal) {
        editSectionModal.addEventListener('click', (e) => {
            if (e.target === editSectionModal) closeEditSection();
        });
    }

    const editSectionForm = document.getElementById('edit-section-form');
    if (editSectionForm) {
        editSectionForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const newName = document.getElementById('edit-section-name').value.trim();
            const newLink = document.getElementById('edit-section-link').value.trim();

            if (!newName) {
                UI.showError('Please enter a section name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-edit-section-btn', true);

            try {
                await client.editSection(resourceId, sectionPosition, newName, newLink);

                // Update the global state
                currentSectionName = newName;

                closeEditSection();
                UI.setButtonLoading('save-edit-section-btn', false);

                // Update the page title and section name display
                document.getElementById('section-name').textContent = newName;
                document.title = `${newName} - Flashback`;

                // Update URL with new name and link
                const currentUrl = new URL(window.location.href);
                currentUrl.searchParams.set('name', newName);
                currentUrl.searchParams.set('sectionLink', newLink);
                window.history.replaceState({}, '', currentUrl);

                // Update breadcrumb and re-render cards to update links with the new section name
                displayBreadcrumb();
                renderCards(currentCardsData);

                UI.showSuccess('Section updated successfully');
            } catch (err) {
                console.error('Edit section failed:', err);
                UI.showError(err.message || 'Failed to edit section');
                UI.setButtonLoading('save-edit-section-btn', false);
            }
        });
    }

    // Remove section handlers
    const removeSectionBtn = document.getElementById('remove-section-btn');
    const removeSectionModal = document.getElementById('remove-section-modal');
    const closeRemoveSectionModalBtn = document.getElementById('close-remove-section-modal-btn');
    const cancelRemoveSectionBtn = document.getElementById('cancel-remove-section-btn');

    const closeRemoveSection = () => {
        UI.toggleElement('remove-section-modal', false);
        document.body.style.overflow = '';
    };

    if (removeSectionBtn) {
        removeSectionBtn.addEventListener('click', () => {
            UI.toggleElement('remove-section-modal', true);
            document.body.style.overflow = 'hidden';
        });
    }
    if (closeRemoveSectionModalBtn) closeRemoveSectionModalBtn.addEventListener('click', closeRemoveSection);
    if (cancelRemoveSectionBtn) cancelRemoveSectionBtn.addEventListener('click', closeRemoveSection);
    if (removeSectionModal) {
        removeSectionModal.addEventListener('click', (e) => {
            if (e.target === removeSectionModal) closeRemoveSection();
        });
    }

    // Setup assign card to topic modal handlers
    const assignTopicModal = document.getElementById('assign-topic-modal');
    if (assignTopicModal) {
        assignTopicModal.addEventListener('click', (e) => {
            if (e.target === assignTopicModal) closeAssignTopicModal();
        });
    }

    const confirmAssignTopicBtn = document.getElementById('confirm-assign-topic-btn');
    if (confirmAssignTopicBtn) {
        confirmAssignTopicBtn.addEventListener('click', assignCardToTopic);
    }

    const confirmCreateAssignTopicBtn = document.getElementById('confirm-create-assign-topic-btn');
    if (confirmCreateAssignTopicBtn) {
        confirmCreateAssignTopicBtn.addEventListener('click', createAndAssignTopic);
    }

    const showCreateTopicBtn = document.getElementById('show-create-topic-btn');
    if (showCreateTopicBtn) {
        showCreateTopicBtn.addEventListener('click', () => {
            UI.toggleElement('topic-search-mode', false);
            UI.toggleElement('topic-create-mode', true);
            UI.toggleElement('confirm-assign-topic-btn', false);
            UI.toggleElement('confirm-create-assign-topic-btn', true);
            document.getElementById('new-topic-name').value = document.getElementById('topic-search-input').value;
            document.getElementById('new-topic-name').focus();
        });
    }

    const backToTopicSearchBtn = document.getElementById('back-to-topic-search-btn');
    if (backToTopicSearchBtn) {
        backToTopicSearchBtn.addEventListener('click', () => {
            UI.toggleElement('topic-search-mode', true);
            UI.toggleElement('topic-create-mode', false);
            UI.toggleElement('confirm-create-assign-topic-btn', false);
            if (currentSelectedTopic) {
                UI.toggleElement('confirm-assign-topic-btn', true);
            }
        });
    }

    const subjectSelectionInfo = document.getElementById('subject-selection-info');
    if (subjectSelectionInfo) {
        subjectSelectionInfo.addEventListener('click', () => {
            UI.toggleElement('subject-selection-info', false);
            UI.toggleElement('subject-search-container', true);
            UI.toggleElement('topics-section', false);
            document.getElementById('subject-search-input').value = '';
            document.getElementById('subject-search-input').focus();
        });
    }

    const topicSearchInput = document.getElementById('topic-search-input');
    if (topicSearchInput) {
        topicSearchInput.addEventListener('input', (e) => {
            const searchTerm = e.target.value.toLowerCase().trim();
            filterTopics(searchTerm);
        });
    }

    const confirmRemoveSectionBtn = document.getElementById('confirm-remove-section-btn');
    if (confirmRemoveSectionBtn) {
        confirmRemoveSectionBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-section-btn', true);

            try {
                await client.removeSection(resourceId, sectionPosition);

                closeRemoveSection();
                UI.setButtonLoading('confirm-remove-section-btn', false);

                // Redirect back to resource page
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
                const milestoneLevel = UI.getUrlParam('level') || '';
                const currentTab = UI.getUrlParam('tab') || 'resources';
                window.location.href = `resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&type=${resourceType}&pattern=${resourcePattern}&link=${encodeURIComponent(resourceLink)}&production=${resourceProduction}&expiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
            } catch (err) {
                console.error('Remove section failed:', err);
                UI.showError(err.message || 'Failed to remove section');
                UI.setButtonLoading('confirm-remove-section-btn', false);
            }
        });
    }

    // Setup move card modal handlers
    const moveCardModal = document.getElementById('move-card-modal');
    const closeMoveCardModalBtn = document.getElementById('close-move-card-modal-btn');
    const cancelMoveCardBtn = document.getElementById('cancel-move-card-btn');

    if (closeMoveCardModalBtn) closeMoveCardModalBtn.addEventListener('click', closeMoveCardModal);
    if (cancelMoveCardBtn) cancelMoveCardBtn.addEventListener('click', closeMoveCardModal);
    if (moveCardModal) {
        moveCardModal.addEventListener('click', (e) => {
            if (e.target === moveCardModal) closeMoveCardModal();
        });
    }

    const confirmMoveCardBtn = document.getElementById('confirm-move-card-btn');
    if (confirmMoveCardBtn) {
        confirmMoveCardBtn.addEventListener('click', async () => {
            if (currentSelectedMoveSection) {
                await moveCardToSection(currentSelectedMoveSection);
            }
        });
    }

    const sectionSearchInput = document.getElementById('section-search-input');
    if (sectionSearchInput) {
        // Interactive search with debouncing
        let searchTimeout = null;
        sectionSearchInput.addEventListener('input', (e) => {
            const searchValue = e.target.value.trim();

            // Clear previous timeout
            if (searchTimeout) {
                clearTimeout(searchTimeout);
            }

            // Hide results if search is empty
            if (searchValue === '') {
                document.getElementById('section-search-results').style.display = 'none';
                document.getElementById('section-search-empty').style.display = 'none';
                document.getElementById('section-search-loading').style.display = 'none';
                const sectionsSection = document.getElementById('resource-sections-section');
                if (sectionsSection) sectionsSection.style.display = 'block';
                return;
            }

            const sectionsSection = document.getElementById('resource-sections-section');
            if (sectionsSection) sectionsSection.style.display = 'none';

            // Show loading state
            document.getElementById('section-search-loading').style.display = 'block';
            document.getElementById('section-search-results').style.display = 'none';
            document.getElementById('section-search-empty').style.display = 'none';

            // Debounce search - wait 300ms after user stops typing
            searchTimeout = setTimeout(async () => {
                await searchForSections(searchValue);
            }, 300);
        });
    }

    // Integrated Resource Selection handlers
    const changeResourceBtn = document.getElementById('change-resource-btn');
    const cancelResourceSelectionBtn = document.getElementById('cancel-resource-selection-btn');
    const resourceSearchInput = document.getElementById('reorder-resource-search-input');

    if (changeResourceBtn) {
        changeResourceBtn.addEventListener('click', openResourceSelectionModal);
    }

    if (cancelResourceSelectionBtn) {
        cancelResourceSelectionBtn.addEventListener('click', closeResourceSelection);
    }

    if (resourceSearchInput) {
        let searchTimeout = null;
        resourceSearchInput.addEventListener('input', (e) => {
            const query = e.target.value.trim();
            if (searchTimeout) clearTimeout(searchTimeout);
            
            const resourcesSection = document.getElementById('subject-resources-section');
            if (query.length > 0) {
                if (resourcesSection) resourcesSection.style.display = 'none';
                searchTimeout = setTimeout(() => searchResources(query), 300);
            } else {
                if (resourcesSection) resourcesSection.style.display = 'block';
                const resultsContainer = document.getElementById('reorder-resource-search-results');
                if (resultsContainer) resultsContainer.innerHTML = '';
            }
        });
    }

    const subjectSearchInput = document.getElementById('subject-search-input');
    if (subjectSearchInput) {
        let searchTimeout = null;
        subjectSearchInput.addEventListener('input', (e) => {
            const searchValue = e.target.value.trim();
            if (searchTimeout) clearTimeout(searchTimeout);

            if (searchValue === '') {
                document.getElementById('subject-search-results').style.display = 'none';
                document.getElementById('subject-search-empty').style.display = 'none';
                document.getElementById('subject-search-loading').style.display = 'none';
                return;
            }

            document.getElementById('subject-search-loading').style.display = 'block';
            document.getElementById('subject-search-results').style.display = 'none';
            document.getElementById('subject-search-empty').style.display = 'none';
            document.getElementById('topics-by-level').innerHTML = '';

            searchTimeout = setTimeout(async () => {
                await searchForSubjects(searchValue);
            }, 300);
        });
    }

    loadCards();
});

let currentSectionName = UI.getUrlParam('name');
let currentResourceId = parseInt(UI.getUrlParam('resourceId'));
let currentSectionPosition = parseInt(UI.getUrlParam('sectionPosition'));
let currentCardsData = [];
let newlyCreatedCardId = null;

// Global state for merge cards
let mergeState = {
    active: false,
    sourceId: null,
    sourceHeadline: null
};

async function loadCards() {
    // Refresh card list to apply merge mode styles if active
    const container = document.getElementById('cards-list');
    if (mergeState.active) {
        document.body.classList.add('merge-mode-active');
    } else {
        document.body.classList.remove('merge-mode-active');
    }

    UI.toggleElement('loading', true);
    UI.toggleElement('cards-list', false);
    UI.toggleElement('empty-state', false);

    try {
        const resourceId = UI.getUrlParam('resourceId');
        const sectionPosition = UI.getUrlParam('sectionPosition');

        const cards = await client.getSectionCards(resourceId, sectionPosition);
        currentCardsData = cards;

        UI.toggleElement('loading', false);

        if (cards.length === 0) {
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('cards-list', true);
            renderCards(cards);
        }
    } catch (err) {
        console.error('Loading cards failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load cards: ' + (err.message || 'Unknown error'));
    }
}

function renderCards(cards) {
    const container = document.getElementById('cards-list');
    container.innerHTML = '';

    const stateNames = ['draft', 'reviewed', 'completed', 'approved', 'released', 'rejected'];
    const sectionName = currentSectionName || UI.getUrlParam('name') || '';
    const resourceName = UI.getUrlParam('resourceName') || '';
    const resourceId = UI.getUrlParam('resourceId') || '';
    const sectionPosition = UI.getUrlParam('sectionPosition') || '';
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';

    cards.forEach(card => {
        const cardItem = document.createElement('div');
        cardItem.className = 'item-block compact';
        cardItem.style.cursor = 'pointer';

        if (newlyCreatedCardId && card.id === newlyCreatedCardId) {
            cardItem.classList.add('newly-created');
            setTimeout(() => {
                cardItem.scrollIntoView({ behavior: 'smooth', block: 'center' });
                newlyCreatedCardId = null; // Clear it so it doesn't re-highlight on refresh/manual reload
            }, 100);
        }

        const stateName = stateNames[card.state] || 'draft';

        // State badge colors
        const stateColors = {
            'draft': { bg: 'rgba(158, 158, 158, 0.2)', color: '#9e9e9e' },
            'reviewed': { bg: 'rgba(33, 150, 243, 0.2)', color: '#2196f3' },
            'completed': { bg: 'rgba(76, 175, 80, 0.2)', color: '#4caf50' },
            'approved': { bg: 'rgba(255, 152, 0, 0.2)', color: '#ff9800' },
            'released': { bg: 'rgba(3, 169, 244, 0.2)', color: '#03a9f4' },
            'rejected': { bg: 'rgba(244, 67, 54, 0.2)', color: '#f44336' }
        };
        const stateColor = stateColors[stateName] || stateColors['draft'];

        const assignButtonHtml = card.isAssignable ? `
                    <button class="block-action-btn block-assign-btn" onclick="event.stopPropagation(); handleAssignToTopic(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"></path>
                            <path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"></path>
                        </svg>
                        Assign
                    </button>
        ` : '';

        const mergeButtonHtml = `
            <button class="block-action-btn block-merge-btn" onclick="event.stopPropagation(); window.enterMergeMode(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M4 12V4a2 2 0 0 1 2-2h10l4 4v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2v-4"></path>
                    <polyline points="14 2 14 6 20 6"></polyline>
                    <path d="M12 18v-6l3 3M12 12l-3 3"></path>
                </svg>
                Merge
            </button>
        `;

        const moveButtonHtml = `
            <button class="block-action-btn block-move-btn" onclick="event.stopPropagation(); handleMoveCard(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 2h9a2 2 0 0 1 2 2z"></path>
                    <path d="M12 11l2 2-2 2"></path>
                    <path d="M8 13h6"></path>
                </svg>
                Move
            </button>
        `;

        cardItem.innerHTML = `
            <div class="item-header" style="margin-bottom: 0;">
                <div style="display: flex; align-items: flex-start; gap: var(--space-xs); flex: 1;" data-card-id="${card.id}" data-card-headline="${UI.escapeHtml(card.headline)}" data-card-state="${card.state}">
                    <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); font-weight: 600; word-break: break-word;">${UI.escapeHtml(card.headline)}</h3>
                </div>
                <div style="display: flex; gap: 0.4rem; align-items: center; justify-content: flex-end; flex-wrap: wrap;">
                    <span class="item-badge" style="background: ${stateColor.bg}; color: ${stateColor.color}; text-transform: capitalize; font-size: 11px; height: 24px; min-width: auto; padding: 0 10px; border-radius: var(--radius-full); display: inline-flex; align-items: center; white-space: nowrap; margin-bottom: 2px;">${UI.escapeHtml(stateName)}</span>
                    ${assignButtonHtml}
                    ${mergeButtonHtml}
                    ${moveButtonHtml}
                </div>
            </div>
        `;

        if (mergeState.active && mergeState.sourceId === card.id) {
            cardItem.classList.add('merge-source');
        }

        cardItem.addEventListener('click', () => {
            if (mergeState.active) {
                if (mergeState.sourceId === card.id) {
                    window.exitMergeMode();
                    return;
                }
                showMergeConfirmModal(card.id, card.headline);
                return;
            }
            const milestoneLevel = UI.getUrlParam('level') || '';
            const sectionState = UI.getUrlParam('sectionState') || '0';
            window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&sectionState=${sectionState}&practiceMode=selective&resourceName=${encodeURIComponent(resourceName)}&sectionName=${encodeURIComponent(sectionName)}&resourceId=${resourceId}&sectionPosition=${sectionPosition}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&level=${milestoneLevel}`;
        });

        container.appendChild(cardItem);
    });

    // Apply KaTeX auto-render to all card headlines
    if (typeof renderMathInElement !== 'undefined') {
        renderMathInElement(container, {
            delimiters: [
                {left: '$$', right: '$$', display: true},
                {left: '$', right: '$', display: false}
            ],
            throwOnError: false,
            errorColor: 'var(--color-error)'
        });
    }
}

function displayBreadcrumb() {
    const resourceId = UI.getUrlParam('resourceId');
    const resourceName = UI.getUrlParam('resourceName');
    const subjectId = UI.getUrlParam('subjectId');
    const subjectName = UI.getUrlParam('subjectName');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');
    const level = UI.getUrlParam('level');
    const currentTab = UI.getUrlParam('tab') || 'resources';
    const sectionPosition = UI.getUrlParam('sectionPosition');
    const sectionName = currentSectionName || UI.getUrlParam('name');

    const breadcrumbItems = [];

    if (roadmapId && roadmapName) {
        breadcrumbItems.push({
            name: roadmapName,
            icon: UI.getRoadmapIcon(),
            url: `roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}`
        });
    }

    if (subjectId && subjectName) {
        breadcrumbItems.push({
            name: subjectName,
            url: `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${level || ''}&tab=${currentTab}`
        });
    }

    if (resourceId && resourceName) {
        const resourceType = UI.getUrlParam('resourceType') || '0';
        const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
        const resourceLink = UI.getUrlParam('resourceLink') || '';
        const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
        const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
        
        breadcrumbItems.push({
            name: resourceName,
            icon: UI.getResourceIcon(parseInt(resourceType)),
            url: `resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&type=${resourceType}&pattern=${resourcePattern}&link=${encodeURIComponent(resourceLink)}&production=${resourceProduction}&expiration=${resourceExpiration}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${level || ''}&tab=${currentTab}`
        });
    }

    UI.renderBreadcrumbs(breadcrumbItems);
}

// Global state for move card modal
let currentMovingCardId = null;
let currentMovingCardHeadline = null;
let currentSelectedMoveSection = null;
let moveState = {
    targetResourceId: null,
    targetResourceName: null,
    subjectId: UI.getUrlParam('subjectId')
};

let currentAssigningCardId = null;
let currentAssigningCardHeadline = null;
let currentSelectedSubjectId = null;
let currentSelectedTopic = null;
let currentSubjectTopics = [];

window.handleAssignToTopic = async function(cardId, cardHeadline) {
    currentAssigningCardId = cardId;
    currentAssigningCardHeadline = cardHeadline;

    const modal = document.getElementById('assign-topic-modal');
    UI.toggleElement('assign-topic-modal', true);
    document.body.style.overflow = 'hidden';
    document.getElementById('assigning-card-headline').textContent = cardHeadline;
    if (typeof renderMathInElement !== 'undefined') {
        renderMathInElement(document.getElementById('assigning-card-headline'), {
            delimiters: [
                {left: '$$', right: '$$', display: true},
                {left: '$', right: '$', display: false}
            ],
            throwOnError: false,
            errorColor: 'var(--color-error)'
        });
    }
    
    // Reset subject search
    document.getElementById('subject-search-input').value = '';
    document.getElementById('subject-search-results').style.display = 'none';
    document.getElementById('subject-search-empty').style.display = 'none';
    document.getElementById('subject-search-loading').style.display = 'none';
    document.getElementById('subjects-list').innerHTML = '';

    UI.toggleElement('subject-selection-info', false);
    UI.toggleElement('subject-search-container', true);
    UI.toggleElement('topics-section', false);
    document.getElementById('topic-search-input').value = '';
    document.getElementById('topic-search-empty').style.display = 'none';

    document.getElementById('topics-loading').style.display = 'block';
    document.getElementById('topics-by-level').innerHTML = '';
    UI.toggleElement('confirm-assign-topic-btn', false);
    currentSelectedTopic = null;
    currentSubjectTopics = [];

    const subjectId = UI.getUrlParam('subjectId');
    const subjectName = UI.getUrlParam('subjectName');
    currentSelectedSubjectId = subjectId;

    if (subjectId) {
        UI.toggleElement('subject-selection-info', true);
        UI.toggleElement('subject-search-container', false);
        UI.toggleElement('topics-section', true);
        document.getElementById('selected-subject-name').textContent = subjectName || 'Current Subject';
        try {
            await loadTopicsForSubject(subjectId);
        } catch (err) {
            console.error('Failed to load topics:', err);
            document.getElementById('topics-loading').style.display = 'none';
            UI.showError('Failed to load topics: ' + (err.message || 'Unknown error'));
        }
    }
};

async function loadTopicsForSubject(subjectId) {
    document.getElementById('topics-loading').style.display = 'block';
    document.getElementById('topics-by-level').innerHTML = '';
    UI.toggleElement('confirm-assign-topic-btn', false);
    currentSelectedTopic = null;
    currentSubjectTopics = [];
    document.getElementById('topic-search-input').value = '';
    document.getElementById('topic-search-empty').style.display = 'none';
    
    try {
        const allTopics = [];
        for (let level = 0; level <= 2; level++) {
            const levelTopics = await client.getTopics(subjectId, level);
            allTopics.push(...levelTopics);
        }

        currentSubjectTopics = allTopics;
        document.getElementById('topics-loading').style.display = 'none';

        if (allTopics.length === 0) {
            document.getElementById('topics-by-level').innerHTML = '<p class="text-muted">No topics available in this subject.</p>';
        } else {
            renderTopicsForAssignment(allTopics);
        }
    } catch (err) {
        document.getElementById('topics-loading').style.display = 'none';
        throw err;
    }
}

function filterTopics(searchTerm) {
    if (!currentSubjectTopics) return;

    const filteredTopics = currentSubjectTopics.filter(topic => 
        topic.name.toLowerCase().includes(searchTerm)
    );

    if (filteredTopics.length === 0) {
        document.getElementById('topics-by-level').style.display = 'none';
        document.getElementById('topic-search-empty').style.display = 'block';
    } else {
        document.getElementById('topics-by-level').style.display = 'block';
        document.getElementById('topic-search-empty').style.display = 'none';
        renderTopicsForAssignment(filteredTopics);
    }
}

async function searchForSubjects(searchToken) {
    if (!searchToken) return;

    try {
        const subjects = await client.searchSubjects(searchToken);
        document.getElementById('subject-search-loading').style.display = 'none';

        if (subjects.length === 0) {
            document.getElementById('subject-search-results').style.display = 'none';
            document.getElementById('subject-search-empty').style.display = 'block';
            return;
        }

        displaySubjectResults(subjects);
        document.getElementById('subject-search-results').style.display = 'block';
        document.getElementById('subject-search-empty').style.display = 'none';
    } catch (err) {
        console.error('Failed to search subjects:', err);
        document.getElementById('subject-search-loading').style.display = 'none';
        document.getElementById('subject-search-empty').style.display = 'block';
    }
}

function displaySubjectResults(subjects) {
    const container = document.getElementById('subjects-list');
    container.innerHTML = '';

    const searchTerm = document.getElementById('subject-search-input').value.trim();

    subjects.forEach(subject => {
        const item = document.createElement('div');
        item.className = 'subject-result-item search-result-item';

        let highlightedName = UI.escapeHtml(subject.name);
        if (searchTerm) {
            const regex = new RegExp(`(${searchTerm.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
            highlightedName = highlightedName.replace(regex, '<mark style="background-color: #fff59d; padding: 0 2px; border-radius: 2px;">$1</mark>');
        }

        item.innerHTML = `
            <div style="font-weight: 600; color: var(--color-text-primary);">${highlightedName}</div>
            <div class="topic-item-meta">Subject</div>
        `;
        item.addEventListener('click', async () => {
            currentSelectedSubjectId = subject.id;
            currentSelectedTopic = null;
            UI.toggleElement('confirm-assign-topic-btn', false);
            document.getElementById('subject-search-results').style.display = 'none';
            
            UI.toggleElement('subject-selection-info', true);
            UI.toggleElement('subject-search-container', false);
            UI.toggleElement('topics-section', true);
            document.getElementById('selected-subject-name').textContent = subject.name;
            
            await loadTopicsForSubject(subject.id);
        });

        container.appendChild(item);
    });
}

function renderTopicsForAssignment(topics) {
    const container = document.getElementById('topics-by-level');
    container.innerHTML = '';

    const searchTerm = document.getElementById('topic-search-input').value.toLowerCase().trim();

    const levelNames = ['Surface', 'Depth', 'Origin'];
    
    // Group by level
    const grouped = topics.reduce((acc, topic) => {
        if (!acc[topic.level]) acc[topic.level] = [];
        acc[topic.level].push(topic);
        return acc;
    }, {});

    // For each level, sort and render
    for (let level = 0; level <= 2; level++) {
        if (grouped[level] && grouped[level].length > 0) {
            const levelDiv = document.createElement('div');
            levelDiv.className = 'topic-level-group';
            
            const levelHeader = document.createElement('div');
            levelHeader.className = 'topic-level-header';
            levelHeader.innerHTML = `
                <span class="level-badge level-badge-${level}">${levelNames[level]}</span>
            `;
            levelDiv.appendChild(levelHeader);

            // Sort by position
            grouped[level].sort((a, b) => a.position - b.position);

            grouped[level].forEach(topic => {
                const topicItem = document.createElement('div');
                topicItem.className = 'topic-item search-result-item';

                let highlightedName = UI.escapeHtml(topic.name);
                if (searchTerm) {
                    const regex = new RegExp(`(${searchTerm.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
                    highlightedName = highlightedName.replace(regex, '<mark style="background-color: #fff59d; padding: 0 2px; border-radius: 2px;">$1</mark>');
                }

                topicItem.innerHTML = `
                    <div class="topic-item-name">
                        <span style="color: var(--color-primary-start); margin-right: 0.5rem; opacity: 0.7;">●</span>
                        ${highlightedName}
                    </div>
                    <div class="topic-pos-badge">Pos ${topic.position}</div>
                `;
                topicItem.addEventListener('click', () => {
                    // Update selection
                    document.querySelectorAll('.topic-item').forEach(el => el.classList.remove('selected'));
                    topicItem.classList.add('selected');
                    currentSelectedTopic = topic;

                    // Show confirmation button
                    const confirmBtn = document.getElementById('confirm-assign-topic-btn');
                    if (confirmBtn) {
                        confirmBtn.style.display = 'block';
                        confirmBtn.scrollIntoView({ behavior: 'smooth', block: 'center' });
                    }
                });
                levelDiv.appendChild(topicItem);
            });

            container.appendChild(levelDiv);
        }
    }
}

async function assignCardToTopic() {
    if (!currentSelectedTopic) return;
    UI.setButtonLoading('confirm-assign-topic-btn', true);
    try {
        const topicName = currentSelectedTopic.name;
        await client.addCardToTopic(currentAssigningCardId, currentSelectedSubjectId, currentSelectedTopic.position, currentSelectedTopic.level);
        closeAssignTopicModal();
        UI.showSuccess(`Card assigned to topic "${topicName}" successfully`);
        await loadCards();
    } catch (err) {
        console.error('Failed to assign card to topic:', err);
        UI.showError('Failed to assign card to topic: ' + (err.message || 'Unknown error'));
    } finally {
        UI.setButtonLoading('confirm-assign-topic-btn', false);
    }
}

async function createAndAssignTopic() {
    const topicName = document.getElementById('new-topic-name').value.trim();
    if (!topicName) {
        UI.showError('Please enter a topic name');
        return;
    }

    const level = parseInt(document.querySelector('input[name="new-topic-level"]:checked').value);
    
    UI.setButtonLoading('confirm-create-assign-topic-btn', true);

    try {
        // 1. Create the topic
        // passing 0 as position to append it at the end
        const newTopic = await client.createTopic(currentSelectedSubjectId, topicName, level, 0);

        // 2. Assign card to the new topic
        await client.addCardToTopic(currentAssigningCardId, currentSelectedSubjectId, newTopic.position, newTopic.level);

        closeAssignTopicModal();
        UI.showSuccess(`Topic "${topicName}" created and card assigned successfully`);
        await loadCards();
    } catch (err) {
        console.error('Failed to create and assign topic:', err);
        UI.showError('Failed to create and assign topic: ' + (err.message || 'Unknown error'));
    } finally {
        UI.setButtonLoading('confirm-create-assign-topic-btn', false);
    }
}

function closeAssignTopicModal() {
    UI.toggleElement('assign-topic-modal', false);
    document.body.style.overflow = '';
    currentAssigningCardId = null;
    currentAssigningCardHeadline = null;
    currentSelectedSubjectId = null;
    currentSelectedTopic = null;
    currentSubjectTopics = [];
    UI.toggleElement('confirm-assign-topic-btn', false);
    UI.toggleElement('confirm-create-assign-topic-btn', false);
    
    const subjectSearchInput = document.getElementById('subject-search-input');
    if (subjectSearchInput) subjectSearchInput.value = '';
    const subjectSearchResults = document.getElementById('subject-search-results');
    if (subjectSearchResults) subjectSearchResults.style.display = 'none';
    const subjectsList = document.getElementById('subjects-list');
    if (subjectsList) subjectsList.innerHTML = '';
    
    const topicSearchInput = document.getElementById('topic-search-input');
    if (topicSearchInput) topicSearchInput.value = '';
    const topicsByLevel = document.getElementById('topics-by-level');
    if (topicsByLevel) topicsByLevel.innerHTML = '';
    const topicSearchEmpty = document.getElementById('topic-search-empty');
    if (topicSearchEmpty) topicSearchEmpty.style.display = 'none';

    UI.toggleElement('topic-search-mode', true);
    UI.toggleElement('topic-create-mode', false);
    const newTopicName = document.getElementById('new-topic-name');
    if (newTopicName) newTopicName.value = '';

    UI.toggleElement('subject-selection-info', false);
    UI.toggleElement('subject-search-container', true);
    UI.toggleElement('topics-section', false);
}

window.closeAssignTopicModal = closeAssignTopicModal;
window.enterMergeMode = function(cardId, headline) {
    mergeState.active = true;
    mergeState.sourceId = cardId;
    mergeState.sourceHeadline = headline;
    
    // Add hint at the top of the list
    const hint = document.createElement('div');
    hint.id = 'reorder-hint';
    hint.className = 'reorder-hint';
    hint.style.background = '#48bb78'; // Use green for merge hint
    hint.innerHTML = `
        <span style="font-weight: 600; font-size: 0.95rem;">Select another card to merge into</span>
        <button class="btn btn-secondary btn-sm" onclick="window.exitMergeMode()" style="padding: 0.4rem 1.5rem; font-size: 0.85rem; background: rgba(255,255,255,0.2); border: none; white-space: nowrap;">Cancel</button>
    `;
    document.body.appendChild(hint);
    
    loadCards();
};

window.exitMergeMode = function() {
    mergeState.active = false;
    mergeState.sourceId = null;
    mergeState.sourceHeadline = null;
    document.body.classList.remove('merge-mode-active');
    
    const hint = document.getElementById('reorder-hint');
    if (hint) {
        hint.remove();
    }
    
    loadCards();
};

function showMergeConfirmModal(targetId, targetHeadline) {
    const modal = document.getElementById('merge-cards-confirm-modal');
    const sourceHeadlineText = document.getElementById('merge-source-headline-text');
    const targetHeadlineText = document.getElementById('merge-target-headline-text');
    const customHeadlineInput = document.getElementById('merge-custom-headline-input');
    const confirmBtn = document.getElementById('confirm-merge-cards-btn');

    sourceHeadlineText.textContent = mergeState.sourceHeadline;
    targetHeadlineText.textContent = targetHeadline;

    // Reset modal state
    customHeadlineInput.style.display = 'none';
    customHeadlineInput.value = '';
    confirmBtn.style.display = 'none';
    document.querySelectorAll('.headline-option').forEach(option => option.classList.remove('selected'));
    document.querySelectorAll('input[name="merge-headline-option"]').forEach(radio => radio.checked = false);

    // Render KaTeX if needed
    if (typeof renderMathInElement !== 'undefined') {
        renderMathInElement(sourceHeadlineText, { delimiters: [{left: '$$', right: '$$', display: true}, {left: '$', right: '$', display: false}], throwOnError: false });
        renderMathInElement(targetHeadlineText, { delimiters: [{left: '$$', right: '$$', display: true}, {left: '$', right: '$', display: false}], throwOnError: false });
    }

    UI.toggleElement('merge-cards-confirm-modal', true);
    document.body.style.overflow = 'hidden';

    // Event listeners for this modal session
    const handleHeadlineOptionChange = (e) => {
        // Update selected class
        document.querySelectorAll('.headline-option').forEach(option => {
            const radio = option.querySelector('input[type="radio"]');
            if (radio.checked) {
                option.classList.add('selected');
            } else {
                option.classList.remove('selected');
            }
        });

        if (e.target.value === 'custom') {
            customHeadlineInput.style.display = 'block';
            customHeadlineInput.focus();
            confirmBtn.style.display = customHeadlineInput.value.trim() ? 'block' : 'none';
        } else {
            customHeadlineInput.style.display = 'none';
            confirmBtn.style.display = 'block';
        }
    };

    const handleCustomHeadlineInput = () => {
        confirmBtn.style.display = customHeadlineInput.value.trim() ? 'block' : 'none';
    };

    const handleConfirm = async () => {
        const selectedOption = document.querySelector('input[name="merge-headline-option"]:checked').value;
        let finalHeadline = '';
        if (selectedOption === 'source') finalHeadline = mergeState.sourceHeadline;
        else if (selectedOption === 'target') finalHeadline = targetHeadline;
        else finalHeadline = customHeadlineInput.value.trim();

        UI.setButtonLoading('confirm-merge-cards-btn', true);
        try {
            await client.mergeCards(mergeState.sourceId, targetId, finalHeadline);
            window.exitMergeMode();
            closeMergeConfirmModal();
            UI.showSuccess('Cards merged successfully');
        } catch (err) {
            console.error('Failed to merge cards:', err);
            UI.showError('Failed to merge cards: ' + (err.message || 'Unknown error'));
        } finally {
            UI.setButtonLoading('confirm-merge-cards-btn', false);
        }
    };

    const closeMergeConfirmModal = () => {
        UI.toggleElement('merge-cards-confirm-modal', false);
        document.body.style.overflow = '';
        // Cleanup listeners
        document.querySelectorAll('input[name="merge-headline-option"]').forEach(radio => radio.removeEventListener('change', handleHeadlineOptionChange));
        customHeadlineInput.removeEventListener('input', handleCustomHeadlineInput);
        confirmBtn.removeEventListener('click', handleConfirm);
    };

    document.querySelectorAll('input[name="merge-headline-option"]').forEach(radio => radio.addEventListener('change', handleHeadlineOptionChange));
    customHeadlineInput.addEventListener('input', handleCustomHeadlineInput);
    confirmBtn.addEventListener('click', handleConfirm);

    document.getElementById('close-merge-cards-modal-btn').onclick = closeMergeConfirmModal;
    document.getElementById('cancel-merge-cards-btn').onclick = closeMergeConfirmModal;

    if (modal) {
        modal.onclick = (e) => {
            if (e.target === modal) closeMergeConfirmModal();
        };
    }
}

window.handleMoveCard = function(cardId, cardHeadline) {
    currentMovingCardId = cardId;
    currentMovingCardHeadline = cardHeadline;
    currentSelectedMoveSection = null;

    // Initialize move state with current resource
    moveState.targetResourceId = parseInt(UI.getUrlParam('resourceId'));
    moveState.targetResourceName = UI.getUrlParam('resourceName');
    updateMoveCardTargetResourceUI();

    UI.toggleElement('move-card-modal', true);
    UI.toggleElement('move-card-info', true);
    document.body.style.overflow = 'hidden';
    document.getElementById('moving-card-headline').textContent = cardHeadline;
    if (typeof renderMathInElement !== 'undefined') {
        renderMathInElement(document.getElementById('moving-card-headline'), {
            delimiters: [
                {left: '$$', right: '$$', display: true},
                {left: '$', right: '$', display: false}
            ],
            throwOnError: false,
            errorColor: 'var(--color-error)'
        });
    }
    document.getElementById('section-search-input').value = '';
    document.getElementById('section-search-results').style.display = 'none';
    document.getElementById('section-search-empty').style.display = 'none';
    document.getElementById('section-search-loading').style.display = 'none';
    document.getElementById('sections-list').innerHTML = '';
    UI.toggleElement('confirm-move-card-btn', false);
    
    // Load initial sections for the current resource
    loadResourceSections();

    setTimeout(() => {
        document.getElementById('section-search-input').focus();
    }, 100);
};

function closeMoveCardModal() {
    UI.toggleElement('move-card-modal', false);
    document.body.style.overflow = '';

    UI.toggleElement('move-card-info', true);
    UI.toggleElement('resource-selection-section', false);
    UI.toggleElement('section-selection-section', true);

    currentMovingCardId = null;
    currentMovingCardHeadline = null;
    currentSelectedMoveSection = null;
    document.getElementById('section-search-input').value = '';
    document.getElementById('section-search-results').style.display = 'none';
    document.getElementById('section-search-empty').style.display = 'none';
    document.getElementById('section-search-loading').style.display = 'none';
    document.getElementById('sections-list').innerHTML = '';
    UI.toggleElement('confirm-move-card-btn', false);
}

function updateMoveCardTargetResourceUI() {
    const targetResourceNameElem = document.getElementById('target-resource-name');
    if (targetResourceNameElem) {
        targetResourceNameElem.textContent = moveState.targetResourceName;
    }
}

function openResourceSelectionModal() {
    UI.toggleElement('move-card-info', false);
    UI.toggleElement('section-selection-section', false);
    UI.toggleElement('resource-selection-section', true);
    UI.toggleElement('confirm-move-card-btn', false);

    const searchInput = document.getElementById('reorder-resource-search-input');
    if (searchInput) {
        searchInput.value = '';
        searchInput.focus();
    }
    const container = document.getElementById('reorder-resource-search-results');
    if (container) {
        container.innerHTML = '';
    }

    const resourcesContainer = document.getElementById('subject-resources-list');
    const resourcesSection = document.getElementById('subject-resources-section');
    if (resourcesContainer) resourcesContainer.innerHTML = '';
    
    if (moveState.subjectId) {
        if (resourcesSection) resourcesSection.style.display = 'block';
        loadSubjectResources();
    } else {
        if (resourcesSection) resourcesSection.style.display = 'none';
    }
}

function closeResourceSelection() {
    UI.toggleElement('move-card-info', true);
    UI.toggleElement('resource-selection-section', false);
    UI.toggleElement('section-selection-section', true);
    if (currentSelectedMoveSection) {
        UI.toggleElement('confirm-move-card-btn', true);
    }
}

async function loadSubjectResources() {
    try {
        const resources = await client.getResources(moveState.subjectId);
        displaySubjectResourceResults(resources);
    } catch (err) {
        console.error('Load subject resources failed:', err);
    }
}

function displaySubjectResourceResults(resources) {
    const container = document.getElementById('subject-resources-list');
    if (!container) return;
    container.innerHTML = '';
    
    if (resources.length === 0) {
        container.innerHTML = '<div class="no-results" style="padding: 1rem; text-align: center; opacity: 0.6;">No resources in this subject.</div>';
        return;
    }

    resources.forEach(res => {
        const item = document.createElement('div');
        item.className = 'search-result-item';
        const icon = UI.getResourceIcon(res.type);
        item.innerHTML = `
            <div class="search-result-name"><span class="resource-icon" style="margin-right: 8px;">${icon}</span> ${UI.escapeHtml(res.name)}</div>
        `;
        
        item.onclick = async () => {
            closeResourceSelection();
            updateTargetResource(res.id, res.name);
        };
        
        container.appendChild(item);
    });
}

function updateTargetResource(id, name) {
    moveState.targetResourceId = id;
    moveState.targetResourceName = name;
    
    const targetNameEl = document.getElementById('target-resource-name');
    if (targetNameEl) targetNameEl.textContent = name;
    
    // Clear previous search and load sections for the new resource
    const searchInput = document.getElementById('section-search-input');
    if (searchInput) searchInput.value = '';
    
    currentSelectedMoveSection = null;
    UI.toggleElement('confirm-move-card-btn', false);
    
    loadResourceSections();
}

async function loadResourceSections() {
    const section = document.getElementById('resource-sections-section');
    const container = document.getElementById('resource-sections-list');
    const resultsContainer = document.getElementById('section-search-results');
    
    if (!moveState.targetResourceId) {
        if (section) section.style.display = 'none';
        return;
    }

    if (section) section.style.display = 'block';
    if (resultsContainer) resultsContainer.style.display = 'none';
    if (container) container.innerHTML = '<div style="padding: 1rem; text-align: center;"><div class="loading" style="width: 20px; height: 20px; border-width: 2px; margin: 0 auto;"></div></div>';

    try {
        const sections = await client.getSections(moveState.targetResourceId);
        displayResourceSections(sections);
    } catch (err) {
        console.error('Load resource sections failed:', err);
        if (container) container.innerHTML = '<div class="no-results">Failed to load sections.</div>';
    }
}

function displayResourceSections(sections) {
    const container = document.getElementById('resource-sections-list');
    if (!container) return;
    container.innerHTML = '';

    if (sections.length === 0) {
        container.innerHTML = '<div class="no-results" style="padding: 1rem; text-align: center; opacity: 0.6;">No sections in this resource.</div>';
        return;
    }

    sections.forEach(section => {
        if (moveState.targetResourceId === currentResourceId && section.position === currentSectionPosition) {
            return;
        }

        const item = document.createElement('div');
        item.className = 'search-result-item';
        if (currentSelectedMoveSection && currentSelectedMoveSection.position === section.position) {
            item.classList.add('selected');
        }

        item.innerHTML = `
            <div class="search-result-name">${UI.escapeHtml(section.name)}</div>
        `;
        
        item.onclick = () => {
            selectMoveTarget(section);
            container.querySelectorAll('.search-result-item').forEach(el => el.classList.remove('selected'));
            item.classList.add('selected');
        };
        
        container.appendChild(item);
    });
}

function selectMoveTarget(section) {
    currentSelectedMoveSection = section;
    
    // Show confirm button
    const confirmBtn = document.getElementById('confirm-move-card-btn');
    if (confirmBtn) {
        confirmBtn.style.display = 'block';
        confirmBtn.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
}

async function searchResources(query) {
    const resultsContainer = document.getElementById('reorder-resource-search-results');
    if (!resultsContainer) return;

    if (!query || query.trim().length === 0) {
        resultsContainer.innerHTML = '';
        return;
    }

    resultsContainer.innerHTML = '<div style="text-align: center; padding: 1rem;"><div class="loading" style="width: 24px; height: 24px; border-width: 3px; margin: 0 auto;"></div></div>';

    try {
        const resources = await client.searchResources(query);
        displayResourceResults(resources);
    } catch (err) {
        console.error('Failed to search resources:', err);
        resultsContainer.innerHTML = '<div style="text-align: center; padding: 1rem; color: var(--danger-color);">Failed to search resources</div>';
    }
}

function displayResourceResults(resources) {
    const resultsContainer = document.getElementById('reorder-resource-search-results');
    if (!resultsContainer) return;

    if (!resources || resources.length === 0) {
        resultsContainer.innerHTML = '<div style="text-align: center; padding: 1rem; color: var(--text-muted);">No resources found</div>';
        return;
    }

    resultsContainer.innerHTML = '';
    resources.forEach(resource => {
        const item = document.createElement('div');
        item.className = 'search-result-item';

        item.innerHTML = `
            <div class="search-result-name">${UI.escapeHtml(resource.name)}</div>
        `;

        item.onclick = () => {
            closeResourceSelection();
            updateTargetResource(resource.id, resource.name);
            document.getElementById('section-search-input').focus();
        };

        resultsContainer.appendChild(item);
    });
}

async function searchForSections(searchToken) {
    if (!searchToken) {
        return;
    }

    currentSelectedMoveSection = null;
    UI.toggleElement('confirm-move-card-btn', false);

    const resourceId = moveState.targetResourceId;

    try {
        const sections = await client.searchSections(resourceId, searchToken);

        // Hide loading
        document.getElementById('section-search-loading').style.display = 'none';

        if (sections.length === 0) {
            document.getElementById('section-search-results').style.display = 'none';
            document.getElementById('section-search-empty').style.display = 'block';
            return;
        }

        // Display sections
        displaySectionResults(sections);
        document.getElementById('section-search-results').style.display = 'block';
        document.getElementById('section-search-empty').style.display = 'none';
    } catch (err) {
        console.error('Failed to search sections:', err);
        document.getElementById('section-search-loading').style.display = 'none';
        document.getElementById('section-search-empty').style.display = 'block';
    }
}

function displaySectionResults(sections) {
    const container = document.getElementById('sections-list');
    container.innerHTML = '';

    const searchInput = document.getElementById('section-search-input');
    const searchTerm = searchInput.value.trim();

    sections.forEach(section => {
        if (moveState.targetResourceId === currentResourceId && section.position === currentSectionPosition) {
            return;
        }

        const sectionItem = document.createElement('div');
        sectionItem.className = 'section-list-item';
        if (currentSelectedMoveSection && currentSelectedMoveSection.position === section.position) {
            sectionItem.classList.add('selected');
        }

        // Highlight matching text
        let highlightedName = UI.escapeHtml(section.name);
        if (searchTerm) {
            const regex = new RegExp(`(${searchTerm.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
            highlightedName = highlightedName.replace(regex, '<mark style="background-color: #fff59d; padding: 0 2px; border-radius: 2px;">$1</mark>');
        }

        sectionItem.innerHTML = `
            <div style="font-weight: 600;">${highlightedName}</div>
        `;

        sectionItem.addEventListener('click', () => {
            selectMoveTarget(section);
            container.querySelectorAll('.section-list-item').forEach(item => {
                item.classList.remove('selected');
            });
            sectionItem.classList.add('selected');
        });

        container.appendChild(sectionItem);
    });
}

async function moveCardToSection(targetSection) {
    if (!targetSection) return;

    UI.setButtonLoading('confirm-move-card-btn', true);

    const resourceId = parseInt(UI.getUrlParam('resourceId'));
    const currentSectionPosition = parseInt(UI.getUrlParam('sectionPosition'));

    try {
        await client.moveCardToSection(currentMovingCardId, resourceId, currentSectionPosition, moveState.targetResourceId, targetSection.position);

        // Close modal and reload cards
        closeMoveCardModal();
        UI.showSuccess(`Card moved to "${targetSection.name}" successfully`);
        await loadCards();
    } catch (err) {
        console.error('Failed to move card:', err);
        UI.showError('Failed to move card: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('confirm-move-card-btn', false);
    }
}
