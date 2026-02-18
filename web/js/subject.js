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

    document.getElementById('subject-name').textContent = subjectName || 'Subject';
    document.title = `${subjectName || 'Subject'} - Flashback`;

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    // Tab switching
    const tabs = document.querySelectorAll('.tab');
    const tabContents = document.querySelectorAll('.tab-content');

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const targetTab = tab.dataset.tab;

            // Update active tab
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');

            // Update active content
            tabContents.forEach(content => {
                content.classList.remove('active');
            });
            document.getElementById(`${targetTab}-content`).classList.add('active');

            // Load resources when switching to resources tab
            if (targetTab === 'resources' && !resourcesLoaded) {
                loadResources();
            }
        });
    });

    // Resources functionality
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
            const pattern = document.getElementById('resource-pattern').value;
            const url = document.getElementById('resource-url').value.trim();
            const productionDate = document.getElementById('resource-production').value;
            const expirationDate = document.getElementById('resource-expiration').value;

            if (!name || !type || !pattern || !url || !productionDate || !expirationDate) {
                UI.showError('Please fill in all fields');
                return;
            }

            // Convert dates to epoch seconds
            const production = Math.floor(new Date(productionDate).getTime() / 1000);
            const expiration = Math.floor(new Date(expirationDate).getTime() / 1000);

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-resource-btn', true);

            try {
                const resource = await client.createResource(name, parseInt(type), parseInt(pattern), url, production, expiration);
                await client.addResourceToSubject(subjectId, resource.id);

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

    // Load resources initially if on resources tab
    let resourcesLoaded = false;
    loadResources();
});

async function loadResources() {
    UI.toggleElement('resources-loading', true);
    UI.toggleElement('resources-list', false);
    UI.toggleElement('resources-empty-state', false);

    try {
        const subjectId = UI.getUrlParam('id');
        const resources = await client.getResources(subjectId);

        UI.toggleElement('resources-loading', false);
        resourcesLoaded = true;

        if (resources.length === 0) {
            UI.toggleElement('resources-empty-state', true);
        } else {
            UI.toggleElement('resources-list', true);
            renderResources(resources);
        }
    } catch (err) {
        console.error('Loading resources failed:', err);
        UI.toggleElement('resources-loading', false);
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
