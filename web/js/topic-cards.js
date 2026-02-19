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
        breadcrumbHtml += `<a href="subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`;
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
        cardItem.style.cursor = 'pointer';

        const stateName = stateNames[card.state] || 'draft';

        cardItem.innerHTML = `
            <div class="card-headline">${UI.escapeHtml(card.headline)}</div>
            <span class="card-state ${stateName}">${UI.escapeHtml(stateName)}</span>
        `;

        cardItem.addEventListener('click', () => {
            window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(card.headline)}&state=${card.state}&practiceMode=selective&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&topicName=${encodeURIComponent(topicName)}`;
        });

        container.appendChild(cardItem);
    });
}
