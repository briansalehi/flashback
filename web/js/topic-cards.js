function displayBreadcrumb() {
    const breadcrumb = document.getElementById('breadcrumb');
    if (!breadcrumb) {
        console.error('Breadcrumb element not found');
        return;
    }

    const subjectId = UI.getUrlParam('subjectId');
    const subjectName = UI.getUrlParam('subjectName');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');

    let breadcrumbHtml = '';

    if (roadmapId && roadmapName) {
        breadcrumbHtml += `<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`;
    }

    if (subjectId && subjectName) {
        if (breadcrumbHtml) breadcrumbHtml += ' → ';
        const topicLevel = UI.getUrlParam('topicLevel') || '0';
        const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;
        const currentTab = UI.getUrlParam('tab') || 'topics';
        breadcrumbHtml += `<a href="subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${milestoneLevel}&tab=${currentTab}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`;
    }

    if (breadcrumbHtml) {
        breadcrumb.innerHTML = breadcrumbHtml;
        breadcrumb.style.display = 'block';
    } else {
        console.log('No breadcrumb HTML generated');
    }
}

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const subjectId = parseInt(UI.getUrlParam('subjectId'));
    const topicPosition = parseInt(UI.getUrlParam('topicPosition'));
    const topicLevel = parseInt(UI.getUrlParam('topicLevel'));
    const topicName = UI.getUrlParam('name');

    if (isNaN(subjectId) || isNaN(topicPosition) || isNaN(topicLevel)) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('topic-name').textContent = topicName || 'Topic';
    document.title = `${topicName || 'Topic'} - Flashback`;

    // Display topic level badge
    const levelNames = ['Surface', 'Depth', 'Origin'];
    const levelBadge = document.getElementById('topic-level-badge');
    if (levelBadge) {
        levelBadge.textContent = levelNames[topicLevel] || 'Unknown';
    }

    // Display breadcrumb
    displayBreadcrumb();

    // Reveal edit/remove on title click
    const topicTitle = document.getElementById('topic-name');
    if (topicTitle) {
        topicTitle.setAttribute('tabindex', '0');
        const editBtn = document.getElementById('edit-topic-btn');
        const removeBtn = document.getElementById('remove-topic-btn');
        const reveal = () => {
            if (editBtn) editBtn.style.display = 'inline-block';
            if (removeBtn) removeBtn.style.display = 'inline-block';
        };
        topicTitle.addEventListener('click', reveal);
        topicTitle.addEventListener('keydown', (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); reveal(); }});
    }

    // Edit topic handlers
    const editTopicBtn = document.getElementById('edit-topic-btn');
    const editTopicModal = document.getElementById('edit-topic-modal');
    const closeEditTopicModalBtn = document.getElementById('close-edit-topic-modal-btn');
    const cancelEditTopicBtn = document.getElementById('cancel-edit-topic-btn');

    const closeEditTopic = () => {
        UI.toggleElement('edit-topic-modal', false);
        document.body.style.overflow = '';
        UI.clearForm('edit-topic-form');
    };

    if (editTopicBtn) {
        editTopicBtn.addEventListener('click', () => {
            UI.toggleElement('edit-topic-modal', true);
            document.body.style.overflow = 'hidden';
            document.getElementById('edit-topic-name').value = topicName || '';
            document.getElementById('edit-topic-level').value = topicLevel;
            setTimeout(() => {
                document.getElementById('edit-topic-name').focus();
            }, 100);
        });
    }
    if (closeEditTopicModalBtn) closeEditTopicModalBtn.addEventListener('click', closeEditTopic);
    if (cancelEditTopicBtn) cancelEditTopicBtn.addEventListener('click', closeEditTopic);
    if (editTopicModal) {
        editTopicModal.addEventListener('click', (e) => {
            if (e.target === editTopicModal) closeEditTopic();
        });
    }

    const editTopicForm = document.getElementById('edit-topic-form');
    if (editTopicForm) {
        editTopicForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const newName = document.getElementById('edit-topic-name').value.trim();
            const newLevel = parseInt(document.getElementById('edit-topic-level').value);

            if (!newName) {
                UI.showError('Please enter a topic name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-edit-topic-btn', true);

            try {
                await client.editTopic(subjectId, topicLevel, topicPosition, newName, newLevel);

                // Update the page title and topic name display
                document.getElementById('topic-name').textContent = newName;
                document.title = `${newName} - Flashback`;

                // Update level badge
                const levelNames = ['Surface', 'Depth', 'Origin'];
                const levelBadge = document.getElementById('topic-level-badge');
                if (levelBadge) {
                    levelBadge.textContent = levelNames[newLevel] || 'Unknown';
                }

                // Update URL with new name and level
                const currentUrl = new URL(window.location.href);
                currentUrl.searchParams.set('name', newName);
                currentUrl.searchParams.set('topicLevel', newLevel);
                window.history.replaceState({}, '', currentUrl);

                closeEditTopic();
                UI.setButtonLoading('save-edit-topic-btn', false);

                UI.showSuccess('Topic updated successfully');
            } catch (err) {
                console.error('Edit topic failed:', err);
                UI.showError(err.message || 'Failed to edit topic');
                UI.setButtonLoading('save-edit-topic-btn', false);
            }
        });
    }

    // Remove topic handlers
    const removeTopicBtn = document.getElementById('remove-topic-btn');
    const removeTopicModal = document.getElementById('remove-topic-modal');
    const closeRemoveTopicModalBtn = document.getElementById('close-remove-topic-modal-btn');
    const cancelRemoveTopicBtn = document.getElementById('cancel-remove-topic-btn');

    const closeRemoveTopic = () => {
        UI.toggleElement('remove-topic-modal', false);
        document.body.style.overflow = '';
    };

    if (removeTopicBtn) {
        removeTopicBtn.addEventListener('click', () => {
            UI.toggleElement('remove-topic-modal', true);
            document.body.style.overflow = 'hidden';
        });
    }
    if (closeRemoveTopicModalBtn) closeRemoveTopicModalBtn.addEventListener('click', closeRemoveTopic);
    if (cancelRemoveTopicBtn) cancelRemoveTopicBtn.addEventListener('click', closeRemoveTopic);
    if (removeTopicModal) {
        removeTopicModal.addEventListener('click', (e) => {
            if (e.target === removeTopicModal) closeRemoveTopic();
        });
    }

    const confirmRemoveTopicBtn = document.getElementById('confirm-remove-topic-btn');
    if (confirmRemoveTopicBtn) {
        confirmRemoveTopicBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-topic-btn', true);

            try {
                await client.removeTopic(subjectId, topicLevel, topicPosition);

                closeRemoveTopic();
                UI.setButtonLoading('confirm-remove-topic-btn', false);

                // Redirect back to subject page
                const subjectName = UI.getUrlParam('subjectName') || '';
                const roadmapId = UI.getUrlParam('roadmapId') || '';
                const roadmapName = UI.getUrlParam('roadmapName') || '';
                const currentTab = UI.getUrlParam('tab') || 'topics';
                // Pass the milestone level so subject page knows which levels to display
                const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;
                window.location.href = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}&tab=${currentTab}`;
            } catch (err) {
                console.error('Remove topic failed:', err);
                UI.showError(err.message || 'Failed to remove topic');
                UI.setButtonLoading('confirm-remove-topic-btn', false);
            }
        });
    }

    // Setup move card modal handlers
    const moveCardModal = document.getElementById('move-card-modal');
    const closeMoveCardModalBtn = document.getElementById('close-move-card-modal-btn');
    const cancelMoveCardBtn = document.getElementById('cancel-move-card-btn');

    if (closeMoveCardModalBtn) {
        closeMoveCardModalBtn.addEventListener('click', () => {
            closeMoveCardModal();
        });
    }

    if (cancelMoveCardBtn) {
        cancelMoveCardBtn.addEventListener('click', () => {
            closeMoveCardModal();
        });
    }

    if (moveCardModal) {
        moveCardModal.addEventListener('click', (e) => {
            if (e.target === moveCardModal) {
                closeMoveCardModal();
            }
        });
    }

    const confirmMoveCardBtn = document.getElementById('confirm-move-card-btn');
    if (confirmMoveCardBtn) {
        confirmMoveCardBtn.addEventListener('click', async () => {
            if (currentSelectedMoveTarget) {
                await moveCardToTopic(currentSelectedMoveTarget);
            }
        });
    }

    const topicSearchInput = document.getElementById('topic-search-input');
    if (topicSearchInput) {
        // Interactive search with debouncing
        let searchTimeout = null;
        topicSearchInput.addEventListener('input', (e) => {
            const searchValue = e.target.value.trim();

            // Clear previous timeout
            if (searchTimeout) {
                clearTimeout(searchTimeout);
            }

            // Hide results if search is empty
            if (searchValue === '') {
                document.getElementById('topic-search-results').style.display = 'none';
                document.getElementById('topic-search-empty').style.display = 'none';
                document.getElementById('topic-search-loading').style.display = 'none';
                return;
            }

            // Show loading state
            document.getElementById('topic-search-loading').style.display = 'block';
            document.getElementById('topic-search-results').style.display = 'none';
            document.getElementById('topic-search-empty').style.display = 'none';

            // Debounce search - wait 300ms after user stops typing
            searchTimeout = setTimeout(async () => {
                await searchForTopics(searchValue);
            }, 300);
        });
    }

    loadCards();
    loadAssessments();

    // Add Assessment button
    const addAssessmentBtn = document.getElementById('add-assessment-btn');
    const assessmentModal = document.getElementById('add-assessment-modal');
    const closeAssessmentModalBtn = document.getElementById('close-assessment-modal-btn');
    const cancelAssessmentBtn = document.getElementById('cancel-assessment-btn');

    let subjectAssessments = [];
    let selectedAssessmentId = null;

    if (addAssessmentBtn) {
        addAssessmentBtn.addEventListener('click', async () => {
            UI.toggleElement('add-assessment-modal', true);
            document.body.style.overflow = 'hidden';
            
            // Default to "Select Existing" tab
            switchToSelectExisting();
            
            // Load existing assessments
            await loadSubjectAssessments();
        });
    }

    const switchToSelectExisting = () => {
        document.getElementById('tab-select-existing').style.borderBottom = '2px solid var(--accent-color)';
        document.getElementById('tab-select-existing').style.color = 'var(--accent-color)';
        document.getElementById('tab-select-existing').style.fontWeight = '600';
        
        document.getElementById('tab-create-new').style.borderBottom = 'none';
        document.getElementById('tab-create-new').style.color = 'var(--text-muted)';
        document.getElementById('tab-create-new').style.fontWeight = '400';
        
        UI.toggleElement('select-existing-section', true);
        UI.toggleElement('assessment-form', false);
    };

    const switchToCreateNew = () => {
        document.getElementById('tab-create-new').style.borderBottom = '2px solid var(--accent-color)';
        document.getElementById('tab-create-new').style.color = 'var(--accent-color)';
        document.getElementById('tab-create-new').style.fontWeight = '600';
        
        document.getElementById('tab-select-existing').style.borderBottom = 'none';
        document.getElementById('tab-select-existing').style.color = 'var(--text-muted)';
        document.getElementById('tab-select-existing').style.fontWeight = '400';
        
        UI.toggleElement('select-existing-section', false);
        UI.toggleElement('assessment-form', true);
        document.getElementById('assessment-headline').focus();
    };

    const tabSelectExisting = document.getElementById('tab-select-existing');
    const tabCreateNew = document.getElementById('tab-create-new');

    if (tabSelectExisting) tabSelectExisting.addEventListener('click', switchToSelectExisting);
    if (tabCreateNew) tabCreateNew.addEventListener('click', switchToCreateNew);

    const loadSubjectAssessments = async () => {
        const milestoneLevel = parseInt(UI.getUrlParam('milestoneLevel') || UI.getUrlParam('topicLevel'));
        UI.toggleElement('existing-assessments-loading', true);
        UI.toggleElement('existing-assessments-list', false);
        UI.toggleElement('existing-assessments-empty', false);
        UI.toggleElement('confirm-existing-assessment-btn', false);
        selectedAssessmentId = null;

        try {
            subjectAssessments = await client.getSubjectAssessments(subjectId, milestoneLevel);
            UI.toggleElement('existing-assessments-loading', false);
            
            if (subjectAssessments.length === 0) {
                UI.toggleElement('existing-assessments-empty', true);
                // Auto switch to create new if no existing assessments
                switchToCreateNew();
            } else {
                UI.toggleElement('existing-assessments-list', true);
                renderSubjectAssessments(subjectAssessments);
            }
        } catch (err) {
            console.error('Failed to load subject assessments:', err);
            UI.toggleElement('existing-assessments-loading', false);
            UI.showError('Failed to load existing assessments');
        }
    };

    const renderSubjectAssessments = (assessments) => {
        const container = document.getElementById('existing-assessments-list');
        container.innerHTML = '';
        
        assessments.forEach(assessment => {
            const item = document.createElement('div');
            item.className = 'assessment-item';
            item.style.padding = '1rem';
            item.style.borderBottom = '1px solid var(--border-color)';
            item.style.cursor = 'pointer';
            item.style.transition = 'background-color 0.2s';
            item.dataset.id = assessment.id;
            
            item.innerHTML = `
                <div style="font-weight: 500; color: var(--text-primary);">${UI.escapeHtml(assessment.headline)}</div>
                <div style="font-size: 0.8rem; color: var(--text-muted); margin-top: 0.25rem;">ID: ${assessment.id}</div>
            `;
            
            item.addEventListener('click', () => {
                // Remove previous selection
                container.querySelectorAll('.assessment-item').forEach(el => {
                    el.style.backgroundColor = '';
                    el.style.borderLeft = 'none';
                });
                
                // Set new selection
                item.style.backgroundColor = 'rgba(var(--accent-color-rgb), 0.1)';
                item.style.borderLeft = '4px solid var(--accent-color)';
                selectedAssessmentId = assessment.id;
                UI.toggleElement('confirm-existing-assessment-btn', true);
            });
            
            container.appendChild(item);
        });
    };

    const searchInput = document.getElementById('existing-assessment-search');
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            const term = e.target.value.toLowerCase();
            const filtered = subjectAssessments.filter(a => 
                a.headline.toLowerCase().includes(term) || a.id.toString().includes(term)
            );
            
            renderSubjectAssessments(filtered);
            UI.toggleElement('existing-assessments-empty', filtered.length === 0);
            UI.toggleElement('confirm-existing-assessment-btn', false);
            selectedAssessmentId = null;
        });
    }

    const confirmExistingBtn = document.getElementById('confirm-existing-assessment-btn');
    if (confirmExistingBtn) {
        confirmExistingBtn.addEventListener('click', async () => {
            if (!selectedAssessmentId) return;
            
            UI.setButtonLoading('confirm-existing-assessment-btn', true);
            try {
                await client.expandAssessment(selectedAssessmentId, subjectId, topicLevel, topicPosition);
                UI.showSuccess('Assessment expanded successfully');
                closeAssessmentModal();
                loadAssessments();
            } catch (err) {
                console.error('Failed to expand assessment:', err);
                UI.showError(err.message || 'Failed to expand assessment');
            } finally {
                UI.setButtonLoading('confirm-existing-assessment-btn', false);
            }
        });
    }

    const cancelExistingBtn = document.getElementById('cancel-existing-assessment-btn');
    if (cancelExistingBtn) {
        cancelExistingBtn.addEventListener('click', () => {
            closeAssessmentModal();
        });
    }

    const closeAssessmentModal = () => {
        UI.toggleElement('add-assessment-modal', false);
        document.body.style.overflow = 'auto';
        UI.clearForm('assessment-form');
        if (searchInput) searchInput.value = '';
        selectedAssessmentId = null;
    };

    if (closeAssessmentModalBtn) {
        closeAssessmentModalBtn.addEventListener('click', closeAssessmentModal);
    }

    if (cancelAssessmentBtn) {
        cancelAssessmentBtn.addEventListener('click', closeAssessmentModal);
    }

    if (assessmentModal) {
        assessmentModal.addEventListener('click', (e) => {
            if (e.target === assessmentModal) {
                closeAssessmentModal();
            }
        });
    }

    // Assessment Form Submit
    const assessmentForm = document.getElementById('assessment-form');
    if (assessmentForm) {
        assessmentForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const headline = document.getElementById('assessment-headline').value.trim();

            if (!headline) {
                UI.showError('Please provide a headline');
                return;
            }

            UI.setButtonLoading('save-assessment-btn', true);

            try {
                // First create a card
                const card = await client.createCard(headline);

                // Then create an assessment using the card ID
                await client.createAssessment(card.id, subjectId, topicLevel, topicPosition);

                newlyCreatedCardId = card.id;

                UI.showSuccess('Assessment added successfully');
                closeAssessmentModal();
                UI.setButtonLoading('save-assessment-btn', false);

                loadAssessments();
            } catch (err) {
                console.error('Add assessment failed:', err);
                UI.showError(err.message || 'Failed to add assessment');
                UI.setButtonLoading('save-assessment-btn', false);
            }
        });
    }
});

let newlyCreatedCardId = null;

async function loadCards() {
    UI.toggleElement('loading', true);
    UI.toggleElement('cards-list', false);
    UI.toggleElement('empty-state', false);

    try {
        const subjectId = UI.getUrlParam('subjectId');
        const topicPosition = UI.getUrlParam('topicPosition');
        const topicLevel = UI.getUrlParam('topicLevel');

        const cards = await client.getTopicCards(subjectId, topicPosition, topicLevel);

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
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicPosition = UI.getUrlParam('topicPosition') || '';
    const topicLevel = UI.getUrlParam('topicLevel') || '';
    const topicName = UI.getUrlParam('name') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;

    cards.forEach(card => {
        const cardItem = document.createElement('div');
        cardItem.className = 'item-block compact';
        cardItem.style.cursor = 'pointer';

        if (newlyCreatedCardId && card.id === newlyCreatedCardId) {
            cardItem.classList.add('newly-created');
            setTimeout(() => {
                cardItem.scrollIntoView({ behavior: 'smooth', block: 'center' });
                newlyCreatedCardId = null; // Clear it
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

        cardItem.innerHTML = `
            <div class="item-header" style="margin-bottom: 0;">
                <div style="display: flex; align-items: flex-start; gap: var(--space-xs); flex: 1;" data-card-id="${card.id}" data-card-headline="${UI.escapeHtml(card.headline)}" data-card-state="${card.state}">
                    <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); font-weight: 600; word-break: break-word;">${UI.escapeHtml(card.headline)}</h3>
                </div>
                <div style="display: flex; gap: 0.6rem; align-items: center; justify-content: flex-end; flex-wrap: wrap;">
                    <span class="item-badge" style="background: ${stateColor.bg}; color: ${stateColor.color}; text-transform: capitalize; font-size: 11px; height: 24px; min-width: auto; padding: 0 10px; border-radius: var(--radius-full); display: inline-flex; align-items: center; white-space: nowrap;">${UI.escapeHtml(stateName)}</span>
                    <button class="btn btn-secondary btn-sm" style="padding: 0.3rem 1rem; height: 34px; font-size: 13px; font-weight: 600; white-space: nowrap; min-width: auto;" onclick="event.stopPropagation(); handleMoveCard(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                        Move
                    </button>
                </div>
            </div>
        `;

        cardItem.addEventListener('click', () => {
            window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&practiceMode=selective&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&topicName=${encodeURIComponent(topicName)}&milestoneLevel=${milestoneLevel}`;
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

// Global state for move card modal
let currentMovingCardId = null;
let currentMovingCardHeadline = null;
let currentSelectedMoveTarget = null;

window.handleMoveCard = function(cardId, cardHeadline) {
    currentMovingCardId = cardId;
    currentMovingCardHeadline = cardHeadline;
    currentSelectedMoveTarget = null;

    // Open modal
    UI.toggleElement('move-card-modal', true);
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
    document.getElementById('topic-search-input').value = '';
    document.getElementById('topic-search-results').style.display = 'none';
    document.getElementById('topic-search-empty').style.display = 'none';
    document.getElementById('topic-search-loading').style.display = 'none';
    document.getElementById('topics-list').innerHTML = '';
    UI.toggleElement('confirm-move-card-btn', false);

    setTimeout(() => {
        document.getElementById('topic-search-input').focus();
    }, 100);
};

function closeMoveCardModal() {
    UI.toggleElement('move-card-modal', false);
    document.body.style.overflow = 'auto';

    currentMovingCardId = null;
    currentMovingCardHeadline = null;
    currentSelectedMoveTarget = null;
    document.getElementById('topic-search-input').value = '';
    document.getElementById('topic-search-results').style.display = 'none';
    document.getElementById('topic-search-empty').style.display = 'none';
    document.getElementById('topic-search-loading').style.display = 'none';
    document.getElementById('topics-list').innerHTML = '';
    UI.toggleElement('confirm-move-card-btn', false);
}

async function searchForTopics(searchToken) {
    if (!searchToken) {
        return;
    }

    currentSelectedMoveTarget = null;
    UI.toggleElement('confirm-move-card-btn', false);

    const subjectId = parseInt(UI.getUrlParam('subjectId'));

    try {
        // Search across all levels by trying each level
        const allTopics = [];
        for (let level = 0; level <= 2; level++) {
            try {
                const levelTopics = await client.searchTopics(subjectId, level, searchToken);
                allTopics.push(...levelTopics);
            } catch (err) {
                console.error(`Failed to search topics at level ${level}:`, err);
            }
        }

        // Hide loading
        document.getElementById('topic-search-loading').style.display = 'none';

        if (allTopics.length === 0) {
            document.getElementById('topic-search-results').style.display = 'none';
            document.getElementById('topic-search-empty').style.display = 'block';
            return;
        }

        // Display topics
        displayTopicResults(allTopics);
        document.getElementById('topic-search-results').style.display = 'block';
        document.getElementById('topic-search-empty').style.display = 'none';
    } catch (err) {
        console.error('Failed to search topics:', err);
        document.getElementById('topic-search-loading').style.display = 'none';
        document.getElementById('topic-search-empty').style.display = 'block';
    }
}

function displayTopicResults(topics) {
    const container = document.getElementById('topics-list');
    container.innerHTML = '';

    const searchInput = document.getElementById('topic-search-input');
    const searchTerm = searchInput.value.trim();
    const levelNames = ['Surface', 'Depth', 'Origin'];

    topics.forEach(topic => {
        const topicItem = document.createElement('div');
        topicItem.className = 'topic-list-item';
        if (currentSelectedMoveTarget && currentSelectedMoveTarget.id === topic.id && currentSelectedMoveTarget.level === topic.level && currentSelectedMoveTarget.position === topic.position) {
            topicItem.classList.add('selected');
        }

        // Highlight matching text
        let highlightedName = UI.escapeHtml(topic.name);
        if (searchTerm) {
            const regex = new RegExp(`(${searchTerm.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
            highlightedName = highlightedName.replace(regex, '<mark style="background-color: #fff59d; padding: 0 2px; border-radius: 2px;">$1</mark>');
        }

        topicItem.innerHTML = `
            <div style="font-weight: 600; margin-bottom: 0.25rem;">${highlightedName}</div>
            <div style="font-size: 0.875rem; color: var(--text-muted);">Level: ${levelNames[topic.level] || 'Unknown'} • Position: ${topic.position}</div>
        `;

        topicItem.addEventListener('click', () => {
            // Update selection state
            currentSelectedMoveTarget = topic;

            // Update UI: highlight selected item
            container.querySelectorAll('.topic-list-item').forEach(item => {
                item.classList.remove('selected');
            });
            topicItem.classList.add('selected');

            // Show confirm button
            const confirmBtn = document.getElementById('confirm-move-card-btn');
            if (confirmBtn) {
                confirmBtn.style.display = 'block';
                confirmBtn.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
            }
        });

        container.appendChild(topicItem);
    });
}

async function moveCardToTopic(targetTopic) {
    if (!targetTopic) return;
    
    UI.setButtonLoading('confirm-move-card-btn', true);
    
    const subjectId = parseInt(UI.getUrlParam('subjectId'));
    const currentTopicLevel = parseInt(UI.getUrlParam('topicLevel'));
    const currentTopicPosition = parseInt(UI.getUrlParam('topicPosition'));

    try {
        await client.moveCardToTopic(
            currentMovingCardId,
            subjectId,
            currentTopicLevel,
            currentTopicPosition,
            subjectId, // targetSubjectId (same subject)
            targetTopic.level,
            targetTopic.position
        );

        // Close modal and reload cards
        closeMoveCardModal();
        UI.showSuccess(`Card moved to "${targetTopic.name}" successfully`);
        await loadCards();
    } catch (err) {
        console.error('Failed to move card:', err);
        UI.showError('Failed to move card: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('confirm-move-card-btn', false);
    }
}

async function loadAssessments() {
    const subjectId = parseInt(UI.getUrlParam('subjectId'));
    const topicPosition = parseInt(UI.getUrlParam('topicPosition'));
    const topicLevel = parseInt(UI.getUrlParam('topicLevel'));

    UI.toggleElement('assessments-loading', true);
    UI.toggleElement('assessments-list', false);
    UI.toggleElement('assessments-empty-state', false);

    try {
        const assessments = await client.getAssessments(subjectId, topicLevel, topicPosition);

        UI.toggleElement('assessments-loading', false);

        if (!assessments || assessments.length === 0) {
            UI.toggleElement('assessments-empty-state', true);
        } else {
            UI.toggleElement('assessments-list', true);
            renderAssessments(assessments);
        }
    } catch (err) {
        console.error('Loading assessments failed:', err);
        UI.showError('Failed to load assessments');
        UI.toggleElement('assessments-loading', false);
    }
}

function renderAssessments(assessments) {
    const container = document.getElementById('assessments-list');
    container.innerHTML = '';

    const stateNames = ['draft', 'reviewed', 'completed', 'approved', 'released', 'rejected'];
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicName = UI.getUrlParam('name') || '';
    const topicPosition = UI.getUrlParam('topicPosition') || '';
    const topicLevel = UI.getUrlParam('topicLevel') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;

    assessments.forEach(card => {
        const cardItem = document.createElement('div');
        cardItem.className = 'item-block compact';

        if (newlyCreatedCardId && card.id === newlyCreatedCardId) {
            cardItem.classList.add('newly-created');
            setTimeout(() => {
                cardItem.scrollIntoView({ behavior: 'smooth', block: 'center' });
                newlyCreatedCardId = null; // Clear it
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

        cardItem.innerHTML = `
            <div class="item-header" style="margin-bottom: 0; align-items: center;">
                <div style="display: flex; align-items: center; gap: var(--space-xs); flex: 1; cursor: pointer;" data-card-id="${card.id}" data-card-headline="${UI.escapeHtml(card.headline)}" data-card-state="${card.state}" class="assessment-link">
                    <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); font-weight: 600;">${UI.escapeHtml(card.headline)}</h3>
                </div>
                <div style="display: flex; gap: 0.6rem; align-items: center;">
                    <span class="item-badge" style="background: ${stateColor.bg}; color: ${stateColor.color}; text-transform: capitalize; font-size: 11px; height: 24px; min-width: auto; padding: 0 10px; border-radius: var(--radius-full); display: inline-flex; align-items: center;">${UI.escapeHtml(stateName)}</span>
                    <button class="btn btn-secondary btn-sm" style="background: var(--gradient-primary); color: white; border: none; padding: 0.3rem 1rem; height: 34px; font-size: 13px; font-weight: 600; min-width: auto; white-space: nowrap;" onclick="window.openExpandAssessmentModal(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                        Expand
                    </button>
                    <button class="btn btn-secondary btn-sm" style="background-color: #dc3545; color: white; padding: 0.3rem 1rem; height: 34px; font-size: 13px; font-weight: 600; min-width: auto; white-space: nowrap;" onclick="window.openDiminishAssessmentModal(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                        Diminish
                    </button>
                </div>
            </div>
        `;

        const assessmentLink = cardItem.querySelector('.assessment-link');
        assessmentLink.addEventListener('click', () => {
            window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&topicName=${encodeURIComponent(topicName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&milestoneLevel=${milestoneLevel}`;
        });

        container.appendChild(cardItem);
    });

    // Apply KaTeX auto-render to all assessment headlines
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

    // Diminish Assessment handlers
    const diminishModal = document.getElementById('diminish-assessment-modal');
    const closeDiminishModalBtn = document.getElementById('close-diminish-assessment-modal-btn');
    const cancelDiminishBtn = document.getElementById('cancel-diminish-assessment-btn');
    const confirmDiminishBtn = document.getElementById('confirm-diminish-assessment-btn');
    let assessmentIdToDiminish = null;

    const closeDiminishModal = () => {
        UI.toggleElement('diminish-assessment-modal', false);
        document.body.style.overflow = '';
        assessmentIdToDiminish = null;
    };

    if (closeDiminishModalBtn) closeDiminishModalBtn.addEventListener('click', closeDiminishModal);
    if (cancelDiminishBtn) cancelDiminishBtn.addEventListener('click', closeDiminishModal);
    if (diminishModal) {
        diminishModal.addEventListener('click', (e) => {
            if (e.target === diminishModal) closeDiminishModal();
        });
    }

    if (confirmDiminishBtn) {
        confirmDiminishBtn.addEventListener('click', async () => {
            if (!assessmentIdToDiminish) return;

            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-diminish-assessment-btn', true);

            try {
                const subjectId = parseInt(UI.getUrlParam('subjectId'));
                const topicPosition = parseInt(UI.getUrlParam('topicPosition'));
                const topicLevel = parseInt(UI.getUrlParam('topicLevel'));

                await client.diminishAssessment(assessmentIdToDiminish, subjectId, topicLevel, topicPosition);
                closeDiminishModal();
                UI.setButtonLoading('confirm-diminish-assessment-btn', false);
                loadAssessments();
                UI.showSuccess('Assessment diminished successfully');
            } catch (err) {
                console.error('Failed to diminish assessment:', err);
                UI.showError('Failed to diminish assessment: ' + (err.message || 'Unknown error'));
                UI.setButtonLoading('confirm-diminish-assessment-btn', false);
            }
        });
    }

    window.openDiminishAssessmentModal = (id, headline) => {
        assessmentIdToDiminish = id;
        const nameEl = document.getElementById('assessment-to-diminish-name');
        if (nameEl) nameEl.textContent = headline;
        UI.toggleElement('diminish-assessment-modal', true);
        document.body.style.overflow = 'hidden';
    };

    // Expand Assessment handlers
    const expandModal = document.getElementById('expand-assessment-modal');
    const closeExpandModalBtn = document.getElementById('close-expand-assessment-modal-btn');
    const cancelExpandBtn = document.getElementById('cancel-expand-assessment-btn');
    const confirmExpandBtn = document.getElementById('confirm-expand-assessment-btn');
    const expandTopicsList = document.getElementById('expand-topics-list');
    const expandTopicsSearchInput = document.getElementById('expand-topics-search-input');
    let assessmentIdToExpand = null;
    let selectedTargetTopic = null;
    let cachedTopicsForExpansion = [];
    let coveredTopicsForExpansion = [];
    let currentMaxLevelForExpansion = 0;

    const closeExpandModal = () => {
        UI.toggleElement('expand-assessment-modal', false);
        document.body.style.overflow = '';
        assessmentIdToExpand = null;
        selectedTargetTopic = null;
        cachedTopicsForExpansion = [];
        coveredTopicsForExpansion = [];
        if (expandTopicsSearchInput) expandTopicsSearchInput.value = '';
        UI.toggleElement('confirm-expand-assessment-btn', false);
    };

    if (expandTopicsSearchInput) {
        expandTopicsSearchInput.addEventListener('input', () => {
            const searchTerm = expandTopicsSearchInput.value.trim().toLowerCase();
            const filteredTopics = cachedTopicsForExpansion.filter(topic => 
                topic.name.toLowerCase().includes(searchTerm)
            );
            renderTopicsForExpansion(filteredTopics, currentMaxLevelForExpansion);
        });
    }

    if (closeExpandModalBtn) closeExpandModalBtn.addEventListener('click', closeExpandModal);
    if (cancelExpandBtn) cancelExpandBtn.addEventListener('click', closeExpandModal);
    if (expandModal) {
        expandModal.addEventListener('click', (e) => {
            if (e.target === expandModal) closeExpandModal();
        });
    }

    window.openExpandAssessmentModal = async (id, headline) => {
        assessmentIdToExpand = id;
        const nameEl = document.getElementById('assessment-to-expand-name');
        if (nameEl) nameEl.textContent = headline;
        
        UI.toggleElement('expand-assessment-modal', true);
        document.body.style.overflow = 'hidden';
        
        await loadTopicsForExpansion();
    };

    async function loadTopicsForExpansion() {
        const subjectId = parseInt(UI.getUrlParam('subjectId'));
        const milestoneLevel = parseInt(UI.getUrlParam('milestoneLevel')) || 0;
        currentMaxLevelForExpansion = milestoneLevel;
        
        UI.toggleElement('expand-topics-loading', true);
        UI.toggleElement('expand-topics-search-container', false);
        UI.toggleElement('expand-topics-empty', false);
        expandTopicsList.innerHTML = '';
        UI.toggleElement('confirm-expand-assessment-btn', false);
        
        try {
            const levels = [];
            for (let i = 0; i <= milestoneLevel; i++) {
                levels.push(i);
            }
            
            const [results, coverage] = await Promise.all([
                Promise.all(levels.map(level => client.getTopics(subjectId, level))),
                client.getTopicCoverage(subjectId, assessmentIdToExpand)
            ]);
            
            cachedTopicsForExpansion = results.flat();
            coveredTopicsForExpansion = coverage;
            
            UI.toggleElement('expand-topics-loading', false);
            UI.toggleElement('expand-topics-search-container', cachedTopicsForExpansion.length > 0);
            renderTopicsForExpansion(cachedTopicsForExpansion, milestoneLevel);
        } catch (err) {
            console.error('Failed to load topics for expansion:', err);
            UI.toggleElement('expand-topics-loading', false);
            expandTopicsList.innerHTML = '<p class="text-danger">Failed to load topics.</p>';
        }
    }

    function renderTopicsForExpansion(topics, maxLevel) {
        expandTopicsList.innerHTML = '';
        
        if (topics.length === 0) {
            UI.toggleElement('expand-topics-empty', true);
            return;
        }
        UI.toggleElement('expand-topics-empty', false);

        const levelInfo = [
            { name: 'Surface', color: 'var(--gradient-primary)' },
            { name: 'Depth', color: 'linear-gradient(135deg, #4caf50, #8bc34a)' },
            { name: 'Origin', color: 'linear-gradient(135deg, #ff9800, #ffc107)' }
        ];

        const topicsByLevel = { 0: [], 1: [], 2: [] };
        topics.forEach(topic => {
            if (topicsByLevel[topic.level] !== undefined) {
                topicsByLevel[topic.level].push(topic);
            }
        });

        for (let level = 0; level <= 2; level++) {
            if (topicsByLevel[level] && topicsByLevel[level].length > 0) {
                const levelHeader = document.createElement('div');
                levelHeader.className = 'level-header';
                levelHeader.style.marginTop = '1.5rem';
                levelHeader.style.marginBottom = '1rem';
                levelHeader.innerHTML = `
                    <div style="display: flex; align-items: center; gap: 0.75rem;">
                        <span class="item-badge" style="background-image: ${levelInfo[level].color}; color: white; border: none; font-size: 11px; padding: 2px 10px; border-radius: 4px; font-weight: 700;">${levelInfo[level].name.toUpperCase()}</span>
                        <div style="flex: 1; height: 1px; background: var(--border-color);"></div>
                    </div>
                `;
                expandTopicsList.appendChild(levelHeader);

                const sortedGroup = topicsByLevel[level].sort((a, b) => a.position - b.position);
                sortedGroup.forEach(topic => {
                    const topicItem = document.createElement('div');
                    topicItem.className = 'topic-list-item';
                    topicItem.style.padding = '1rem';
                    topicItem.style.borderRadius = 'var(--radius-md)';
                    topicItem.innerHTML = `
                        <div style="display: flex; align-items: center; justify-content: space-between;">
                            <span style="font-weight: 600; font-size: 1rem;">${UI.escapeHtml(topic.name)}</span>
                            <span class="text-muted" style="font-size: 0.8rem;">Pos: ${topic.position + 1}</span>
                        </div>
                    `;
                    
                    // Don't allow expanding to the current topic or already covered topics
                    const currentTopicPosition = parseInt(UI.getUrlParam('topicPosition'));
                    const currentTopicLevel = parseInt(UI.getUrlParam('topicLevel'));
                    
                    const isCurrentTopic = topic.position === currentTopicPosition && topic.level === currentTopicLevel;
                    const isAlreadyCovered = coveredTopicsForExpansion.some(t => t.position === topic.position && t.level === topic.level);

                    if (isCurrentTopic || isAlreadyCovered) {
                        topicItem.style.opacity = '0.5';
                        topicItem.style.cursor = 'not-allowed';
                        topicItem.title = isCurrentTopic ? 'This is the current topic' : 'This topic is already covered by this assessment';
                    } else {
                        topicItem.addEventListener('click', () => {
                            // Deselect previous
                            const previouslySelected = expandTopicsList.querySelector('.topic-list-item.selected');
                            if (previouslySelected) previouslySelected.classList.remove('selected');
                            
                            topicItem.classList.add('selected');
                            selectedTargetTopic = topic;
                            UI.toggleElement('confirm-expand-assessment-btn', true);
                        });
                    }
                    
                    expandTopicsList.appendChild(topicItem);
                });
            }
        }
    }

    if (confirmExpandBtn) {
        confirmExpandBtn.addEventListener('click', async () => {
            if (!assessmentIdToExpand || !selectedTargetTopic) return;

            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-expand-assessment-btn', true);

            try {
                const subjectId = parseInt(UI.getUrlParam('subjectId'));
                await client.expandAssessment(
                    assessmentIdToExpand, 
                    subjectId, 
                    selectedTargetTopic.level, 
                    selectedTargetTopic.position
                );
                
                closeExpandModal();
                UI.setButtonLoading('confirm-expand-assessment-btn', false);
                UI.showSuccess('Assessment expanded successfully');
                // Optional: reload assessments if needed, but expanding to ANOTHER topic 
                // might not change current topic's assessment list unless it's a new link.
                loadAssessments(); 
            } catch (err) {
                console.error('Failed to expand assessment:', err);
                UI.showError('Failed to expand assessment: ' + (err.message || 'Unknown error'));
                UI.setButtonLoading('confirm-expand-assessment-btn', false);
            }
        });
    }
