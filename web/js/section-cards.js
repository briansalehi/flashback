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
        cardItem.className = 'card-item';
        cardItem.style.cursor = 'pointer';

        const stateName = stateNames[card.state] || 'draft';

        cardItem.innerHTML = `
            <div class="card-headline">${UI.escapeHtml(card.headline)}</div>
            <span class="card-state ${stateName}">${UI.escapeHtml(stateName)}</span>
        `;

        cardItem.addEventListener('click', () => {
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
        breadcrumbHtml += `<a href="resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(resourceName)}</a>`;
    }

    if (breadcrumbHtml) {
        breadcrumb.innerHTML = breadcrumbHtml;
    }
}
