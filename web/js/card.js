window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const cardId = parseInt(UI.getUrlParam('cardId'));
    const headline = UI.getUrlParam('headline');
    const state = parseInt(UI.getUrlParam('state')) || 0;

    if (isNaN(cardId)) {
        window.location.href = '/home.html';
        return;
    }

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
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    loadBlocks();
});

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
