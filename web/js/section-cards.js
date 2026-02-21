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
            stateBadge.className = 'section-state reviewed';
        }

        // Hide the button
        if (markSectionReviewedBtn) {
            markSectionReviewedBtn.style.display = 'none';
        }

        UI.setButtonLoading('mark-section-reviewed-btn', false);
    } catch (err) {
        console.error('Failed to mark section as reviewed:', err);
        UI.showError('Failed to mark section as reviewed: ' + (err.message || 'Unknown error'));
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
    const resourceName = UI.getUrlParam('resourceName');

    if (isNaN(resourceId) || isNaN(sectionPosition)) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('section-name').textContent = sectionName || 'Section';
    document.title = `${sectionName || 'Section'} - Flashback`;

    // Display breadcrumb
    displayBreadcrumb();

    // Display section state badge
    const stateNames = ['draft', 'reviewed', 'completed'];
    const stateName = stateNames[sectionState] || 'draft';
    const stateBadge = document.getElementById('section-state-badge');
    if (stateBadge) {
        stateBadge.textContent = stateName;
        stateBadge.className = `section-state ${stateName}`;
    }

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
                await client.addCardToSection(card.id, resourceId, sectionPosition);

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

    // Setup mark section as reviewed button
    const markSectionReviewedBtn = document.getElementById('mark-section-reviewed-btn');
    if (markSectionReviewedBtn && sectionState !== 1) { // Show only if not already reviewed (state 1)
        markSectionReviewedBtn.style.display = 'inline-block';
        markSectionReviewedBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await markSectionAsReviewed();
        });
    }

    // Edit section handlers
    const editSectionBtn = document.getElementById('edit-section-btn');
    if (editSectionBtn) {
        editSectionBtn.addEventListener('click', () => {
            UI.toggleElement('edit-section-modal', true);
            document.getElementById('edit-section-name').value = sectionName || '';
            // Get the link from URL params - we'll need to add it to the URL params
            const sectionLink = UI.getUrlParam('sectionLink') || '';
            document.getElementById('edit-section-link').value = sectionLink;
            setTimeout(() => {
                document.getElementById('edit-section-name').focus();
            }, 100);
        });
    }

    const cancelEditSectionBtn = document.getElementById('cancel-edit-section-btn');
    if (cancelEditSectionBtn) {
        cancelEditSectionBtn.addEventListener('click', () => {
            UI.toggleElement('edit-section-modal', false);
            UI.clearForm('edit-section-form');
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

                // Update the page title and section name display
                document.getElementById('section-name').textContent = newName;
                document.title = `${newName} - Flashback`;

                // Update URL with new name and link
                const currentUrl = new URL(window.location.href);
                currentUrl.searchParams.set('name', newName);
                currentUrl.searchParams.set('sectionLink', newLink);
                window.history.replaceState({}, '', currentUrl);

                UI.toggleElement('edit-section-modal', false);
                UI.clearForm('edit-section-form');
                UI.setButtonLoading('save-edit-section-btn', false);

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
    if (removeSectionBtn) {
        removeSectionBtn.addEventListener('click', () => {
            UI.toggleElement('remove-section-modal', true);
        });
    }

    const cancelRemoveSectionBtn = document.getElementById('cancel-remove-section-btn');
    if (cancelRemoveSectionBtn) {
        cancelRemoveSectionBtn.addEventListener('click', () => {
            UI.toggleElement('remove-section-modal', false);
        });
    }

    const confirmRemoveSectionBtn = document.getElementById('confirm-remove-section-btn');
    if (confirmRemoveSectionBtn) {
        confirmRemoveSectionBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-section-btn', true);

            try {
                await client.removeSection(resourceId, sectionPosition);

                UI.toggleElement('remove-section-modal', false);
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
                window.location.href = `resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&type=${resourceType}&pattern=${resourcePattern}&link=${encodeURIComponent(resourceLink)}&production=${resourceProduction}&expiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
            } catch (err) {
                console.error('Remove section failed:', err);
                UI.showError(err.message || 'Failed to remove section');
                UI.setButtonLoading('confirm-remove-section-btn', false);
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
                return;
            }

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

    loadCards();
});

async function loadCards() {
    UI.toggleElement('loading', true);
    UI.toggleElement('cards-list', false);
    UI.toggleElement('empty-state', false);

    try {
        const resourceId = UI.getUrlParam('resourceId');
        const sectionPosition = UI.getUrlParam('sectionPosition');

        const cards = await client.getSectionCards(resourceId, sectionPosition);

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
    const sectionName = UI.getUrlParam('name') || '';
    const resourceName = UI.getUrlParam('resourceName') || '';
    const resourceId = UI.getUrlParam('resourceId') || '';
    const sectionPosition = UI.getUrlParam('sectionPosition') || '';
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';

    cards.forEach(card => {
        const cardItem = document.createElement('div');
        cardItem.className = 'item-block';
        cardItem.style.minHeight = 'auto';
        cardItem.style.padding = 'var(--space-md) var(--space-lg)';

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
                <div style="display: flex; align-items: center; gap: var(--space-sm); flex: 1; cursor: pointer;" data-card-id="${card.id}" data-card-headline="${UI.escapeHtml(card.headline)}" data-card-state="${card.state}" class="card-link">
                    <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base);">${UI.escapeHtml(card.headline)}</h3>
                </div>
                <div style="display: flex; gap: var(--space-xs); align-items: center;">
                    <span class="item-badge" style="background: ${stateColor.bg}; color: ${stateColor.color}; text-transform: capitalize; font-size: var(--font-size-xs);">${UI.escapeHtml(stateName)}</span>
                    <button class="btn btn-sm btn-secondary" onclick="handleMoveCard(${card.id}, '${UI.escapeHtml(card.headline).replace(/'/g, "\\'")}')">
                        Move
                    </button>
                </div>
            </div>
        `;

        const cardLink = cardItem.querySelector('.card-link');
        cardLink.addEventListener('click', () => {
            window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&practiceMode=selective&resourceName=${encodeURIComponent(resourceName)}&sectionName=${encodeURIComponent(sectionName)}&resourceId=${resourceId}&sectionPosition=${sectionPosition}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}`;
        });

        container.appendChild(cardItem);
    });
}

function displayBreadcrumb() {
    const breadcrumb = document.getElementById('breadcrumb');
    if (!breadcrumb) return;

    const resourceId = UI.getUrlParam('resourceId');
    const resourceName = UI.getUrlParam('resourceName');
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
        breadcrumbHtml += `<a href="subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`;
    }

    if (resourceId && resourceName) {
        if (breadcrumbHtml) breadcrumbHtml += ' → ';
        const resourceType = UI.getUrlParam('resourceType') || '0';
        const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
        const resourceLink = UI.getUrlParam('resourceLink') || '';
        const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
        const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
        breadcrumbHtml += `<a href="resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&type=${resourceType}&pattern=${resourcePattern}&link=${encodeURIComponent(resourceLink)}&production=${resourceProduction}&expiration=${resourceExpiration}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(resourceName)}</a>`;
    }

    if (breadcrumbHtml) {
        breadcrumb.innerHTML = breadcrumbHtml;
    }
}

// Global state for move card modal
let currentMovingCardId = null;
let currentMovingCardHeadline = null;

window.handleMoveCard = function(cardId, cardHeadline) {
    currentMovingCardId = cardId;
    currentMovingCardHeadline = cardHeadline;

    // Position modal below the dashboard header and scroll to it
    const modal = document.getElementById('move-card-modal');
    const dashboardHeader = document.querySelector('.dashboard-header');

    // Open modal
    modal.style.display = 'block';
    document.getElementById('moving-card-headline').textContent = cardHeadline;
    document.getElementById('section-search-input').value = '';
    document.getElementById('section-search-results').style.display = 'none';
    document.getElementById('section-search-empty').style.display = 'none';
    document.getElementById('section-search-loading').style.display = 'none';
    document.getElementById('sections-list').innerHTML = '';

    // Scroll modal into view smoothly
    setTimeout(() => {
        modal.scrollIntoView({ behavior: 'smooth', block: 'start' });
        document.getElementById('section-search-input').focus();
    }, 100);
};

function closeMoveCardModal() {
    document.getElementById('move-card-modal').style.display = 'none';
    currentMovingCardId = null;
    currentMovingCardHeadline = null;
    document.getElementById('section-search-input').value = '';
    document.getElementById('section-search-results').style.display = 'none';
    document.getElementById('section-search-empty').style.display = 'none';
    document.getElementById('section-search-loading').style.display = 'none';
    document.getElementById('sections-list').innerHTML = '';
}

async function searchForSections(searchToken) {
    if (!searchToken) {
        return;
    }

    const resourceId = parseInt(UI.getUrlParam('resourceId'));

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
        const sectionItem = document.createElement('div');
        sectionItem.className = 'section-list-item';

        // Highlight matching text
        let highlightedName = UI.escapeHtml(section.name);
        if (searchTerm) {
            const regex = new RegExp(`(${searchTerm.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
            highlightedName = highlightedName.replace(regex, '<mark style="background-color: #fff59d; padding: 0 2px; border-radius: 2px;">$1</mark>');
        }

        sectionItem.innerHTML = `
            <div style="font-weight: 600; margin-bottom: 0.25rem;">${highlightedName}</div>
            <div style="font-size: 0.875rem; color: var(--text-muted);">Position: ${section.position}</div>
        `;

        sectionItem.addEventListener('click', async () => {
            await moveCardToSection(section);
        });

        container.appendChild(sectionItem);
    });
}

async function moveCardToSection(targetSection) {
    const resourceId = parseInt(UI.getUrlParam('resourceId'));
    const currentSectionPosition = parseInt(UI.getUrlParam('sectionPosition'));

    try {
        await client.moveCardToSection(currentMovingCardId, resourceId, currentSectionPosition, targetSection.position);

        // Close modal and reload cards
        closeMoveCardModal();
        UI.showSuccess(`Card moved to "${targetSection.name}" successfully`);
        await loadCards();
    } catch (err) {
        console.error('Failed to move card:', err);
        UI.showError('Failed to move card: ' + (err.message || 'Unknown error'));
    }
}
