window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const subjectId = UI.getUrlParam('id');
    const subjectName = UI.getUrlParam('name');
    if (!subjectId) {
        window.location.href = '/home.html';
        return;
    }

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    const addResourceBtn = document.getElementById('add-resource-btn');
    if (addResourceBtn) {
        addResourceBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-resource-form', true);
            setTimeout(() => {
                const nameInput = document.getElementById('resource-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const cancelResourceBtn = document.getElementById('cancel-resource-btn');
    if (cancelResourceBtn) {
        cancelResourceBtn.addEventListener('click', () => {
            UI.toggleElement('add-resource-form', false);
            UI.clearForm('resource-form');
        });
    }

    const resourceForm = document.getElementById('resource-form');
    if (resourceForm) {
        resourceForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('resource-name').value.trim();
            const type = document.getElementById('resource-type').value;
            const url = document.getElementById('resource-url').value.trim();

            if (!name || !type || !url) {
                UI.showError('Please fill in all fields');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-resource-btn', true);

            try {
                await client.addResource(subjectId, name, type, url);

                UI.toggleElement('add-resource-form', false);
                UI.clearForm('resource-form');
                UI.setButtonLoading('save-resource-btn', false);

                loadResources();
            } catch (err) {
                console.error('Add resource failed:', err);
                UI.showError(err.message || 'Failed to add resource');
                UI.setButtonLoading('save-resource-btn', false);
            }
        });
    }

    loadResources();
});

async function loadResources() {
    UI.toggleElement('loading', true);
    UI.toggleElement('resources-list', false);
    UI.toggleElement('empty-state', false);

    try {
        const subjectId = UI.getUrlParam('id');
        const subjectName = UI.getUrlParam('name');
        const resources = await client.getResources(subjectId);

        document.getElementById('subject-name').textContent = subjectName || 'Subject Resources';
        document.title = `${subjectName || 'Resources'} - Flashback`;

        UI.toggleElement('loading', false);

        if (resources.length === 0) {
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('resources-list', true);
            renderResources(resources);
        }
    } catch (err) {
        console.error('Loading resources failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load resources: ' + (err.message || 'Unknown error'));
    }
}

function renderResources(resources) {
    const container = document.getElementById('resources-list');
    container.innerHTML = '';

    resources.forEach(resource => {
        const resourceItem = document.createElement('div');
        resourceItem.className = 'resource-item';
        resourceItem.innerHTML = `
            <div class="resource-header">
                <div class="resource-name">${UI.escapeHtml(resource.name)}</div>
                <span class="resource-type">${UI.escapeHtml(resource.type)}</span>
            </div>
            <div class="resource-url">
                <a href="${UI.escapeHtml(resource.url)}" target="_blank" rel="noopener noreferrer">
                    ${UI.escapeHtml(resource.url)}
                </a>
            </div>
        `;
        container.appendChild(resourceItem);
    });
}
