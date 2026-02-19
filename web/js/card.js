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
        }

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
    const practiceMode = UI.getUrlParam('practiceMode');
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

    // Show context breadcrumb
    displayContextBreadcrumb(practiceMode, subjectName, topicName, resourceName, sectionName);

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

    // Track when user navigates away from card
    // Intercept link clicks to properly record progress before navigation
    if (practiceMode) {
        document.addEventListener('click', async (e) => {
            const link = e.target.closest('a');
            if (link && link.href && cardStartTime) {
                e.preventDefault();
                await handleCardExit();
                window.location.href = link.href;
            }
        });
    }

    // Setup mark as reviewed button
    const markReviewedBtn = document.getElementById('mark-reviewed-btn');
    if (markReviewedBtn && state !== 1) { // Show only if not already reviewed (state 1)
        markReviewedBtn.style.display = 'inline-block';
        markReviewedBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await markCardAsReviewed();
        });
    }

    // Setup edit headline functionality
    setupEditHeadline();

    loadBlocks();
});

// Display context breadcrumb (subject/topic or resource/section)
function displayContextBreadcrumb(practiceMode, subjectName, topicName, resourceName, sectionName) {
    const breadcrumb = document.getElementById('context-breadcrumb');
    if (!breadcrumb) return;

    let contextHtml = '';
    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const localSubjectName = UI.getUrlParam('subjectName') || subjectName || '';

    if (practiceMode === 'aggressive' && subjectName && topicName) {
        // Get subject and roadmap info from practice state or URL
        const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');

        // Get topic info
        const currentTopic = practiceState.topics ? practiceState.topics[practiceState.currentTopicIndex] : null;
        const topicLevel = currentTopic ? currentTopic.level : '';
        const topicPosition = currentTopic ? currentTopic.position : '';

        let breadcrumbParts = [];

        if (roadmapId && roadmapName) {
            breadcrumbParts.push(`<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`);
        }

        if (subjectId && subjectName) {
            const subjectLink = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
            breadcrumbParts.push(`<a href="${subjectLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`);
        }

        if (subjectId && topicLevel !== '' && topicPosition !== '' && topicName) {
            const topicLink = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&name=${encodeURIComponent(topicName)}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
            breadcrumbParts.push(`<a href="${topicLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(topicName)}</a>`);
        }

        contextHtml = breadcrumbParts.join(' → ');
    } else if (practiceMode === 'selective') {
        let breadcrumbParts = [];

        // Always add roadmap if available
        if (roadmapId && roadmapName) {
            breadcrumbParts.push(`<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`);
        }

        // Always add subject if available
        if (subjectId && localSubjectName) {
            const subjectLink = `subject.html?id=${subjectId}&name=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
            breadcrumbParts.push(`<a href="${subjectLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(localSubjectName)}</a>`);
        }

        // Check if coming from topic or resource/section
        const topicPosition = UI.getUrlParam('topicPosition');
        const topicLevel = UI.getUrlParam('topicLevel');
        const localTopicName = UI.getUrlParam('topicName') || topicName || '';

        if (topicPosition !== '' && topicLevel !== '' && localTopicName) {
            // Coming from topic-cards
            const topicLink = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&name=${encodeURIComponent(localTopicName)}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
            breadcrumbParts.push(`<a href="${topicLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(localTopicName)}</a>`);
        } else if (resourceName && sectionName) {
            // Coming from section-cards
            const resourceId = UI.getUrlParam('resourceId') || '';
            const sectionPosition = UI.getUrlParam('sectionPosition') || '';

            if (resourceId && resourceName) {
                const resourceLink = `resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
                breadcrumbParts.push(`<a href="${resourceLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(resourceName)}</a>`);
            }

            if (resourceId && sectionPosition !== '' && sectionName) {
                const sectionLink = `section-cards.html?resourceId=${resourceId}&sectionPosition=${sectionPosition}&name=${encodeURIComponent(sectionName)}&resourceName=${encodeURIComponent(resourceName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
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

// Handle card exit - record progress
async function handleCardExit() {
    if (!cardStartTime) return;

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const practiceMode = UI.getUrlParam('practiceMode');
    const duration = Math.floor((Date.now() - cardStartTime) / 1000); // Convert to seconds

    if (duration >= 3 && practiceMode) {
        try {
            let practiceModeValue;
            if (practiceMode === 'aggressive') {
                practiceModeValue = 0;
            } else if (practiceMode === 'progressive') {
                practiceModeValue = 1;
            } else if (practiceMode === 'selective') {
                practiceModeValue = 2;
            }

            console.log(`making progress on card ${cardId} in ${duration} seconds with practice mode ${practiceModeValue}`);
            await client.makeProgress(cardId, duration, practiceModeValue);

            if (practiceMode === 'selective') {
                console.log(`studying card ${cardId} for ${duration} seconds`);
                await client.study(cardId, duration);
            }
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
        // Check if there's a next topic
        const practiceState = JSON.parse(sessionStorage.getItem('practiceState') || '{}');
        const hasNextTopic = practiceState.topics &&
                             practiceState.currentTopicIndex < practiceState.topics.length - 1;

        const finishBtn = document.createElement('button');
        finishBtn.textContent = hasNextTopic ? 'Next Topic' : 'Finish Practice';
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

    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicName = UI.getUrlParam('topicName') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    window.location.href = `card.html?cardId=${nextCard.id}&headline=${encodeURIComponent(nextCard.headline)}&state=${nextCard.state}&practiceMode=${practiceMode}&cardIndex=${nextIndex}&totalCards=${totalCards}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(topicName)}&roadmapName=${encodeURIComponent(roadmapName)}`;
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
        const subjectName = UI.getUrlParam('subjectName') || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=${practiceMode}&cardIndex=0&totalCards=${cards.length}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(nextTopic.name)}&roadmapName=${encodeURIComponent(roadmapName)}`;
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

function setupEditHeadline() {
    const editBtn = document.getElementById('edit-headline-btn');
    const saveBtn = document.getElementById('save-headline-btn');
    const cancelBtn = document.getElementById('cancel-edit-btn');
    const headlineDisplay = document.getElementById('card-headline');
    const editForm = document.getElementById('edit-headline-form');
    const headlineInput = document.getElementById('headline-input');

    if (!editBtn || !saveBtn || !cancelBtn || !headlineDisplay || !editForm || !headlineInput) {
        return;
    }

    // Edit button click
    editBtn.addEventListener('click', () => {
        headlineInput.value = headlineDisplay.textContent;
        headlineDisplay.style.display = 'none';
        editBtn.style.display = 'none';
        editForm.style.display = 'block';
        headlineInput.focus();
    });

    // Cancel button click
    cancelBtn.addEventListener('click', () => {
        headlineDisplay.style.display = 'block';
        editBtn.style.display = 'block';
        editForm.style.display = 'none';
    });

    // Save button click
    saveBtn.addEventListener('click', async () => {
        const newHeadline = headlineInput.value.trim();

        if (!newHeadline) {
            UI.showError('Headline cannot be empty');
            return;
        }

        if (newHeadline === headlineDisplay.textContent) {
            // No change, just cancel edit mode
            headlineDisplay.style.display = 'block';
            editBtn.style.display = 'block';
            editForm.style.display = 'none';
            return;
        }

        UI.setButtonLoading('save-headline-btn', true);

        try {
            const cardId = parseInt(UI.getUrlParam('cardId'));
            await client.editCard(cardId, newHeadline);

            // Update display
            headlineDisplay.textContent = newHeadline;
            document.title = `${newHeadline} - Flashback`;

            // Exit edit mode
            headlineDisplay.style.display = 'block';
            editBtn.style.display = 'block';
            editForm.style.display = 'none';

            UI.setButtonLoading('save-headline-btn', false);
        } catch (err) {
            console.error('Failed to edit card headline:', err);
            UI.showError('Failed to edit headline: ' + (err.message || 'Unknown error'));
            UI.setButtonLoading('save-headline-btn', false);
        }
    });

    // Allow Enter key to save
    headlineInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            e.preventDefault();
            saveBtn.click();
        }
    });

    // Allow Escape key to cancel
    headlineInput.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            e.preventDefault();
            cancelBtn.click();
        }
    });
}
