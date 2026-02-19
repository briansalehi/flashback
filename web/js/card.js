// Timer to track card reading duration
let cardStartTime = null;

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const headline = UI.getUrlParam('headline');
    const state = parseInt(UI.getUrlParam('state')) || 0;
    const practiceMode = UI.getUrlParam('practiceMode');
    const cardIndex = parseInt(UI.getUrlParam('cardIndex'));
    const totalCards = parseInt(UI.getUrlParam('totalCards'));

    if (isNaN(cardId)) {
        window.location.href = '/home.html';
        return;
    }

    // Start timer
    cardStartTime = Date.now();

    document.getElementById('card-headline').textContent = headline || 'Card';
    document.title = `${headline || 'Card'} - Flashback`;

    const stateNames = ['draft', 'reviewed', 'completed', 'approved', 'released', 'rejected'];
    const stateName = stateNames[state] || 'draft';
    const stateElement = document.getElementById('card-state');
    if (stateElement) {
        stateElement.textContent = stateName;
        stateElement.className = `card-state-display ${stateName}`;
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

    // Add navigation buttons for practice mode
    if (practiceMode && !isNaN(cardIndex) && !isNaN(totalCards)) {
        addPracticeNavigation(practiceMode, cardIndex, totalCards);
    }

    // Track when user navigates away from card (for selective mode without navigation buttons)
    window.addEventListener('beforeunload', () => {
        if (practiceMode && cardStartTime) {
            handleCardExit();
        }
    });

    loadBlocks();
});

// Handle card exit - record progress
async function handleCardExit() {
    if (!cardStartTime) return;

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const practiceMode = UI.getUrlParam('practiceMode');
    const duration = Math.floor((Date.now() - cardStartTime) / 1000); // Convert to seconds

    // Only record if duration is at least 3 seconds
    if (duration >= 3 && practiceMode) {
        try {
            // Map practice mode string to enum value
            const practiceModeValue = practiceMode === 'aggressive' ? 0 : practiceMode === 'progressive' ? 1 : 2;
            await client.makeProgress(cardId, duration, practiceModeValue);
        } catch (err) {
            console.error('Failed to record progress:', err);
        }
    }

    // Reset timer to prevent duplicate calls
    cardStartTime = null;
}

// Add navigation for practice mode
function addPracticeNavigation(practiceMode, cardIndex, totalCards) {
    const contentDiv = document.getElementById('card-content');
    if (!contentDiv) return;

    const navContainer = document.createElement('div');
    navContainer.style.cssText = 'display: flex; justify-content: space-between; align-items: center; margin-top: 2rem; padding-top: 1rem; border-top: 2px solid var(--border-color);';

    const infoText = document.createElement('span');
    infoText.textContent = `Card ${cardIndex + 1} of ${totalCards}`;
    infoText.style.color = 'var(--text-muted)';

    const buttonContainer = document.createElement('div');
    buttonContainer.style.cssText = 'display: flex; gap: 1rem;';

    if (cardIndex + 1 < totalCards) {
        const nextBtn = document.createElement('button');
        nextBtn.textContent = 'Next Card';
        nextBtn.className = 'btn btn-primary';
        nextBtn.addEventListener('click', async () => {
            await handleCardExit();
            await loadNextCard(practiceMode, cardIndex, totalCards);
        });
        buttonContainer.appendChild(nextBtn);
    } else {
        const finishBtn = document.createElement('button');
        finishBtn.textContent = 'Finish Topic';
        finishBtn.className = 'btn btn-accent';
        finishBtn.addEventListener('click', async () => {
            await handleCardExit();
            await loadNextTopic(practiceMode);
        });
        buttonContainer.appendChild(finishBtn);
    }

    navContainer.appendChild(infoText);
    navContainer.appendChild(buttonContainer);
    contentDiv.appendChild(navContainer);
}

// Load next card in current topic
async function loadNextCard(practiceMode, currentIndex, totalCards) {
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

    window.location.href = `card.html?cardId=${nextCard.id}&headline=${encodeURIComponent(nextCard.headline)}&state=${nextCard.state}&practiceMode=${practiceMode}&cardIndex=${nextIndex}&totalCards=${totalCards}`;
}

// Load next topic or finish practice
async function loadNextTopic(practiceMode) {
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
        window.location.href = `/subject.html?id=${practiceState.subjectId}&roadmapId=${practiceState.roadmapId}`;
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
            await loadNextTopic(practiceMode);
            return;
        }

        // Update practice state
        practiceState.currentTopicIndex = nextTopicIndex;
        practiceState.currentCards = cards;
        sessionStorage.setItem('practiceState', JSON.stringify(practiceState));

        // Navigate to first card of next topic
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=${practiceMode}&cardIndex=0&totalCards=${cards.length}`;
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

        UI.toggleElement('loading', false);

        if (blocks.length === 0) {
            UI.toggleElement('card-content', true);
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('card-content', true);
            renderBlocks(blocks);
        }
    } catch (err) {
        console.error('Loading blocks failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load blocks: ' + (err.message || 'Unknown error'));
    }
}

function renderBlocks(blocks) {
    const container = document.getElementById('blocks-list');
    container.innerHTML = '';

    const blockTypes = ['text', 'code', 'image'];

    blocks.forEach(block => {
        const blockItem = document.createElement('div');
        blockItem.className = 'block-item';

        const typeName = blockTypes[block.type] || 'text';

        let metadataHtml = '';
        if (block.metadata) {
            metadataHtml = `<span class="block-metadata">${UI.escapeHtml(block.metadata)}</span>`;
        }

        let extensionHtml = '';
        if (block.extension) {
            extensionHtml = `<span class="block-type">${UI.escapeHtml(block.extension)}</span>`;
        }

        let contentHtml = '';
        if (block.type === 1) { // code
            contentHtml = `<pre class="block-code"><code>${UI.escapeHtml(block.content)}</code></pre>`;
        } else if (block.type === 2) { // image
            contentHtml = `<img src="${UI.escapeHtml(block.content)}" alt="Block image" class="block-image" />`;
        } else { // text (type 0 or default)
            contentHtml = `<div class="block-content">${UI.escapeHtml(block.content)}</div>`;
        }

        blockItem.innerHTML = `
            ${metadataHtml}
            ${extensionHtml}
            ${contentHtml}
        `;

        container.appendChild(blockItem);
    });
}
