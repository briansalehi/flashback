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

    console.log('Breadcrumb params:', { subjectId, subjectName, roadmapId, roadmapName });

    let breadcrumbHtml = '';

    if (roadmapId && roadmapName) {
        breadcrumbHtml += `<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`;
    }

    if (subjectId && subjectName) {
        if (breadcrumbHtml) breadcrumbHtml += ' → ';
        const topicLevel = UI.getUrlParam('topicLevel') || '0';
        const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;
        breadcrumbHtml += `<a href="subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${milestoneLevel}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`;
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

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    const addCardBtn = document.getElementById('add-card-btn');
    if (addCardBtn) {
        addCardBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-card-form', true);
            setTimeout(() => {
                const headlineInput = document.getElementById('card-headline');
                if (headlineInput) {
                    headlineInput.focus();
                }
            }, 100);
        });
    }

    const cancelCardBtn = document.getElementById('cancel-card-btn');
    if (cancelCardBtn) {
        cancelCardBtn.addEventListener('click', () => {
            UI.toggleElement('add-card-form', false);
            UI.clearForm('card-form');
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
                await client.addCardToTopic(card.id, subjectId, topicPosition);

                UI.toggleElement('add-card-form', false);
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

    // Edit topic handlers
    const editTopicBtn = document.getElementById('edit-topic-btn');
    if (editTopicBtn) {
        editTopicBtn.addEventListener('click', () => {
            UI.toggleElement('edit-topic-modal', true);
            document.getElementById('edit-topic-name').value = topicName || '';
            document.getElementById('edit-topic-level').value = topicLevel;
            setTimeout(() => {
                document.getElementById('edit-topic-name').focus();
            }, 100);
        });
    }

    const cancelEditTopicBtn = document.getElementById('cancel-edit-topic-btn');
    if (cancelEditTopicBtn) {
        cancelEditTopicBtn.addEventListener('click', () => {
            UI.toggleElement('edit-topic-modal', false);
            UI.clearForm('edit-topic-form');
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

                UI.toggleElement('edit-topic-modal', false);
                UI.clearForm('edit-topic-form');
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
    if (removeTopicBtn) {
        removeTopicBtn.addEventListener('click', () => {
            UI.toggleElement('remove-topic-modal', true);
        });
    }

    const cancelRemoveTopicBtn = document.getElementById('cancel-remove-topic-btn');
    if (cancelRemoveTopicBtn) {
        cancelRemoveTopicBtn.addEventListener('click', () => {
            UI.toggleElement('remove-topic-modal', false);
        });
    }

    const confirmRemoveTopicBtn = document.getElementById('confirm-remove-topic-btn');
    if (confirmRemoveTopicBtn) {
        confirmRemoveTopicBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-topic-btn', true);

            try {
                await client.removeTopic(subjectId, topicLevel, topicPosition);

                UI.toggleElement('remove-topic-modal', false);
                UI.setButtonLoading('confirm-remove-topic-btn', false);

                // Redirect back to subject page
                const subjectName = UI.getUrlParam('subjectName') || '';
                const roadmapId = UI.getUrlParam('roadmapId') || '';
                const roadmapName = UI.getUrlParam('roadmapName') || '';
                // Pass the milestone level so subject page knows which levels to display
                const milestoneLevel = UI.getUrlParam('milestoneLevel') || topicLevel;
                window.location.href = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${milestoneLevel}`;
            } catch (err) {
                console.error('Remove topic failed:', err);
                UI.showError(err.message || 'Failed to remove topic');
                UI.setButtonLoading('confirm-remove-topic-btn', false);
            }
        });
    }

    // Setup move card modal handlers
    const cancelMoveCardBtn = document.getElementById('cancel-move-card-btn');
    if (cancelMoveCardBtn) {
        cancelMoveCardBtn.addEventListener('click', () => {
            closeMoveCardModal();
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
});

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

    cards.forEach(card => {
        const cardItem = document.createElement('div');
        cardItem.className = 'card-item';

        const stateName = stateNames[card.state] || 'draft';

        cardItem.innerHTML = `
            <div style="display: flex; justify-content: space-between; align-items: center;">
                <div style="flex: 1; cursor: pointer;" data-card-id="${card.id}" data-card-headline="${UI.escapeHtml(card.headline)}" data-card-state="${card.state}" class="card-link">
                    <div class="card-headline">${UI.escapeHtml(card.headline)}</div>
                    <span class="card-state ${stateName}">${UI.escapeHtml(stateName)}</span>
                </div>
                <button class="btn btn-secondary" style="margin-left: 1rem; padding: 0.5rem 1rem;" onclick="handleMoveCard(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                    Move
                </button>
            </div>
        `;

        const cardLink = cardItem.querySelector('.card-link');
        cardLink.addEventListener('click', () => {
            window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&practiceMode=selective&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&topicName=${encodeURIComponent(topicName)}`;
        });

        container.appendChild(cardItem);
    });
}

// Global state for move card modal
let currentMovingCardId = null;
let currentMovingCardHeadline = null;

window.handleMoveCard = function(cardId, cardHeadline) {
    currentMovingCardId = cardId;
    currentMovingCardHeadline = cardHeadline;

    // Open modal
    const modal = document.getElementById('move-card-modal');
    modal.style.display = 'block';
    document.getElementById('moving-card-headline').textContent = cardHeadline;
    document.getElementById('topic-search-input').value = '';
    document.getElementById('topic-search-results').style.display = 'none';
    document.getElementById('topic-search-empty').style.display = 'none';
    document.getElementById('topic-search-loading').style.display = 'none';
    document.getElementById('topics-list').innerHTML = '';

    // Scroll modal into view smoothly
    setTimeout(() => {
        modal.scrollIntoView({ behavior: 'smooth', block: 'start' });
        document.getElementById('topic-search-input').focus();
    }, 100);
};

function closeMoveCardModal() {
    document.getElementById('move-card-modal').style.display = 'none';
    currentMovingCardId = null;
    currentMovingCardHeadline = null;
    document.getElementById('topic-search-input').value = '';
    document.getElementById('topic-search-results').style.display = 'none';
    document.getElementById('topic-search-empty').style.display = 'none';
    document.getElementById('topic-search-loading').style.display = 'none';
    document.getElementById('topics-list').innerHTML = '';
}

async function searchForTopics(searchToken) {
    if (!searchToken) {
        return;
    }

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

        topicItem.addEventListener('click', async () => {
            await moveCardToTopic(topic);
        });

        container.appendChild(topicItem);
    });
}

async function moveCardToTopic(targetTopic) {
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
    }
}
