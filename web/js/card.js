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

    const roadmapId = UI.getUrlParam('roadmapId') || '';
    const roadmapName = UI.getUrlParam('roadmapName') || '';
    const subjectId = UI.getUrlParam('subjectId') || '';
    const subjectName = UI.getUrlParam('subjectName') || '';
    const topicName = UI.getUrlParam('topicName') || '';
    const milestoneId = UI.getUrlParam('milestoneId') || '';
    const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
    window.location.href = `card.html?cardId=${nextCard.id}&headline=${encodeURIComponent(nextCard.headline)}&state=${nextCard.state}&practiceMode=${practiceMode}&cardIndex=${nextIndex}&totalCards=${totalCards}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(topicName)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
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
        const roadmapId = UI.getUrlParam('roadmapId') || practiceState.roadmapId || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        const subjectId = UI.getUrlParam('subjectId') || practiceState.subjectId || '';
        const subjectName = UI.getUrlParam('subjectName') || '';
        const milestoneId = UI.getUrlParam('milestoneId') || '';
        const milestoneLevel = UI.getUrlParam('milestoneLevel') || '';
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=${practiceMode}&cardIndex=0&totalCards=${cards.length}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&topicName=${encodeURIComponent(nextTopic.name)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
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

    const blockTypes = ['text', 'code', 'image'];

    blocks.forEach((block, index) => {
        const blockItem = document.createElement('div');
        blockItem.className = 'content-block';
        blockItem.id = `block-${index}`;
        blockItem.draggable = true;
        blockItem.dataset.position = block.position;

        const typeName = blockTypes[block.type] || 'text';

        let metadataHtml = '';
        if (block.metadata) {
            metadataHtml = `<span class="block-metadata-badge">${UI.escapeHtml(block.metadata)}</span>`;
        }

        let contentHtml = '';
        if (block.type === 1) { // code
            // Map extension to Prism language
            const language = mapExtensionToLanguage(block.extension);
            contentHtml = `<pre class="content-block-text" style="background: rgba(0, 0, 0, 0.3); padding: var(--space-md); border-radius: var(--radius-md); overflow-x: auto;"><code class="language-${language}">${UI.escapeHtml(block.content)}</code></pre>`;
        } else if (block.type === 2) { // image
            contentHtml = `<img src="${UI.escapeHtml(block.content)}" alt="Block image" style="max-width: 100%; height: auto; border-radius: var(--radius-md);" />`;
        } else { // text (type 0 or default)
            contentHtml = `<div class="content-block-text">${UI.escapeHtml(block.content)}</div>`;
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
                <button class="block-action-btn block-remove-btn remove-block-btn" data-position="${block.position}" data-index="${index}">
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <polyline points="3 6 5 6 21 6"></polyline>
                        <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                    </svg>
                    Remove
                </button>
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
            // Don't interfere with buttons, inputs, or textareas
            if (e.target.closest('button') || e.target.closest('input') || e.target.closest('textarea') || e.target.closest('select')) {
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
        });

        blockItem.addEventListener('touchmove', (e) => {
            if (!touchStartElement) return;

            // Calculate movement distance
            const deltaX = Math.abs(e.touches[0].clientX - touchStartX);
            const deltaY = Math.abs(e.touches[0].clientY - touchStartY);

            // If user moved significantly before long-press completed, cancel it (they're scrolling)
            if (!isDragEnabled && (deltaX > 10 || deltaY > 10)) {
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
        });

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

        // Mobile tap to show/hide actions
        let tapTimer = null;
        blockItem.addEventListener('click', (e) => {
            // Don't toggle actions if clicking a button
            if (e.target.closest('.block-action-btn')) {
                return;
            }

            // Toggle actions visibility on mobile
            if (window.innerWidth <= 768) {
                blockItem.classList.toggle('show-actions');
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
