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

    // Setup remove card button - show for section cards only
    const resourceId = UI.getUrlParam('resourceId');
    const sectionPosition = UI.getUrlParam('sectionPosition');
    const removeCardBtn = document.getElementById('remove-card-btn');

    if (removeCardBtn && resourceId && sectionPosition !== '') {
        removeCardBtn.style.display = 'inline-block';
        removeCardBtn.addEventListener('click', (e) => {
            e.preventDefault();
            openRemoveCardModal();
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
    const cancelRemoveBlockBtn = document.getElementById('cancel-remove-block-btn');
    if (cancelRemoveBlockBtn) {
        cancelRemoveBlockBtn.addEventListener('click', () => {
            closeRemoveBlockModal();
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
    if (modalOverlay) {
        modalOverlay.addEventListener('click', () => {
            closeRemoveCardModal();
            closeRemoveBlockModal();
        });
    }

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
                const resourceType = UI.getUrlParam('resourceType') || '0';
                const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
                const resourceLink = UI.getUrlParam('resourceLink') || '';
                const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
                const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
                const resLink = `resource.html?id=${resourceId}&name=${encodeURIComponent(resourceName)}&type=${resourceType}&pattern=${resourcePattern}&link=${encodeURIComponent(resourceLink)}&production=${resourceProduction}&expiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
                breadcrumbParts.push(`<a href="${resLink}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(resourceName)}</a>`);
            }

            if (resourceId && sectionPosition !== '' && sectionName) {
                const resourceType = UI.getUrlParam('resourceType') || '0';
                const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
                const resourceLink = UI.getUrlParam('resourceLink') || '';
                const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
                const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
                const sectionLink = `section-cards.html?resourceId=${resourceId}&sectionPosition=${sectionPosition}&name=${encodeURIComponent(sectionName)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink)}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(localSubjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
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
    const duration = Math.floor((Date.now() - cardStartTime) / 1000); // Convert to seconds

    // Check if this is from a resource/section (study mode) or from topic practice
    const resourceId = UI.getUrlParam('resourceId');
    const sectionPosition = UI.getUrlParam('sectionPosition');
    const milestoneId = UI.getUrlParam('milestoneId');
    const milestoneLevel = UI.getUrlParam('milestoneLevel');

    if (duration >= 3) {
        try {
            // If viewing from resource/section, only call study()
            if (resourceId && sectionPosition !== '') {
                console.log(`studying card ${cardId} for ${duration} seconds`);
                await client.study(cardId, duration);
            }
            // If practicing topics (has milestone info), call makeProgress()
            else if (milestoneId && milestoneLevel !== undefined) {
                console.log(`making progress on card ${cardId} in ${duration} seconds with milestone ${milestoneId}, level ${milestoneLevel}`);
                await client.makeProgress(cardId, duration, parseInt(milestoneId), parseInt(milestoneLevel));
            }
            // Fallback: if neither, just call study
            else {
                console.log(`studying card ${cardId} for ${duration} seconds (fallback)`);
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
    const milestoneId = UI.getUrlParam('milestoneId') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
    window.location.href = `card.html?cardId=${nextCard.id}&headline=${encodeURIComponent(nextCard.headline)}&state=${nextCard.state}&practiceMode=${practiceMode}&cardIndex=${nextIndex}&totalCards=${totalCards}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(topicName)}&roadmapName=${encodeURIComponent(roadmapName)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
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
        const milestoneId = UI.getUrlParam('milestoneId') || '';
        const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=${practiceMode}&cardIndex=0&totalCards=${cards.length}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(nextTopic.name)}&roadmapName=${encodeURIComponent(roadmapName)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
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

    blocks.forEach((block, index) => {
        const blockItem = document.createElement('div');
        blockItem.className = 'block-item';
        blockItem.id = `block-${index}`;
        blockItem.draggable = true;
        blockItem.dataset.position = block.position;

        const typeName = blockTypes[block.type] || 'text';

        // Display mode
        const displayDiv = document.createElement('div');
        displayDiv.className = 'block-display';
        displayDiv.id = `block-display-${index}`;

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

        displayDiv.innerHTML = `
            <div class="block-header">
                <div>
                    ${metadataHtml}
                    ${extensionHtml}
                </div>
                <div class="block-actions">
                    <button class="btn btn-secondary block-action-btn" onclick="editBlock(${index})">Edit</button>
                    <button class="remove-block-btn block-action-btn" data-position="${block.position}" data-index="${index}">Remove</button>
                </div>
            </div>
            ${contentHtml}
        `;

        // Drag and drop handlers
        let isDragging = false;

        blockItem.addEventListener('dragstart', (e) => {
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

        // Remove button handler
        const removeBtn = displayDiv.querySelector('.remove-block-btn');
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
            }
        }, 0);
    });
}

// Global variable to store current blocks
let currentBlocks = [];

window.editBlock = function(index) {
    const displayDiv = document.getElementById(`block-display-${index}`);
    const editDiv = document.getElementById(`block-edit-${index}`);

    if (displayDiv && editDiv) {
        displayDiv.style.display = 'none';
        editDiv.style.display = 'block';
    }
};

window.cancelEditBlock = function(index) {
    const displayDiv = document.getElementById(`block-display-${index}`);
    const editDiv = document.getElementById(`block-edit-${index}`);

    if (displayDiv && editDiv) {
        displayDiv.style.display = 'block';
        editDiv.style.display = 'none';
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
    const newExtension = extensionInput.value.trim();
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
    const newExtension = extensionInput.value.trim();
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

function openRemoveCardModal() {
    document.getElementById('modal-overlay').style.display = 'block';
    document.getElementById('remove-card-modal').style.display = 'block';
}

function closeRemoveCardModal() {
    document.getElementById('modal-overlay').style.display = 'none';
    document.getElementById('remove-card-modal').style.display = 'none';
}

async function confirmRemoveCard() {
    const cardId = parseInt(UI.getUrlParam('cardId'));
    const resourceId = UI.getUrlParam('resourceId');
    const sectionPosition = UI.getUrlParam('sectionPosition');

    if (!cardId) {
        UI.showError('Invalid card ID');
        return;
    }

    UI.setButtonLoading('confirm-remove-card-btn', true);

    try {
        await client.removeCard(cardId);

        // Navigate back to section cards page
        const resourceName = UI.getUrlParam('resourceName') || '';
        const sectionName = UI.getUrlParam('sectionName') || '';
        const resourceType = UI.getUrlParam('resourceType') || '0';
        const resourcePattern = UI.getUrlParam('resourcePattern') || '0';
        const resourceLink = UI.getUrlParam('resourceLink') || '';
        const resourceProduction = UI.getUrlParam('resourceProduction') || '0';
        const resourceExpiration = UI.getUrlParam('resourceExpiration') || '0';
        const subjectId = UI.getUrlParam('subjectId') || '';
        const subjectName = UI.getUrlParam('subjectName') || '';
        const roadmapId = UI.getUrlParam('roadmapId') || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';

        window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${sectionPosition}&name=${encodeURIComponent(sectionName)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink)}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
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
    const overlay = document.getElementById('modal-overlay');

    if (modal && overlay) {
        modal.style.display = 'block';
        overlay.style.display = 'block';

        // Store position in modal for use when confirming
        modal.dataset.blockPosition = position;
    }
}

// Close remove block modal
function closeRemoveBlockModal() {
    const modal = document.getElementById('remove-block-modal');
    const overlay = document.getElementById('modal-overlay');

    if (modal && overlay) {
        modal.style.display = 'none';
        overlay.style.display = 'none';
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
