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

    // Pattern options per resource type
    const patternsByType = {
        0: [{value: 0, label: 'Chapter'}, {value: 1, label: 'Page'}, {value: 7, label: 'Section'}], // Book
        1: [{value: 1, label: 'Page'}], // Website
        2: [{value: 2, label: 'Session'}, {value: 3, label: 'Episode'}], // Course
        3: [{value: 3, label: 'Episode'}], // Video
        4: [{value: 4, label: 'Playlist'}, {value: 5, label: 'Post'}], // Channel
        5: [{value: 5, label: 'Post'}], // Mailing List
        6: [{value: 1, label: 'Page'}], // Manual
        7: [{value: 0, label: 'Chapter'}, {value: 1, label: 'Page'}], // Slides
        8: [{value: 6, label: 'Memory'}], // Nerves
    };

    const typeSelect = document.getElementById('resource-type');
    const patternSelect = document.getElementById('resource-pattern');

    typeSelect.addEventListener('change', () => {
        const typeValue = typeSelect.value;
        patternSelect.innerHTML = '';

        if (!typeValue) {
            patternSelect.disabled = true;
            patternSelect.appendChild(new Option('Select type first...', ''));
            return;
        }

        patternSelect.disabled = false;
        const patterns = patternsByType[parseInt(typeValue)] || [];

        if (patterns.length === 1) {
            patternSelect.appendChild(new Option(patterns[0].label, patterns[0].value));
            patternSelect.value = patterns[0].value;
        } else {
            patternSelect.appendChild(new Option('Select pattern...', ''));
            patterns.forEach(p => {
                patternSelect.appendChild(new Option(p.label, p.value));
            });
        }
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

    function resetPatternSelect() {
        patternSelect.disabled = true;
        patternSelect.innerHTML = '<option value="">Select type first...</option>';
    }

    const cancelResourceBtn = document.getElementById('cancel-resource-btn');
    if (cancelResourceBtn) {
        cancelResourceBtn.addEventListener('click', () => {
            UI.toggleElement('add-resource-form', false);
            UI.clearForm('resource-form');
            resetPatternSelect();
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

            if (!name || !type || !pattern || !productionDate || !expirationDate) {
                UI.showError('Please fill required fields');
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
                resetPatternSelect();
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

    const typeNames = ['Book', 'Website', 'Course', 'Video', 'Channel', 'Mailing List', 'Manual', 'Slides', 'Your Knowledge'];
    const patternNames = ['Chapters', 'Pages', 'Sessions', 'Episodes', 'Playlist', 'Posts', 'Memories'];

    resources.forEach(resource => {
        const resourceItem = document.createElement('div');
        resourceItem.className = 'resource-item';

        // Convert epoch seconds to readable dates
        const productionDate = resource.production ? new Date(resource.production * 1000).toLocaleDateString() : 'N/A';
        const expirationDate = resource.expiration ? new Date(resource.expiration * 1000).toLocaleDateString() : 'N/A';

        const typeName = typeNames[resource.type] || 'Unknown';
        const patternName = patternNames[resource.pattern] || 'Unknown';

        resourceItem.innerHTML = `
            <div class="resource-header">
                <div class="resource-name">${UI.escapeHtml(resource.name)}</div>
                <span class="resource-type">${UI.escapeHtml(typeName)} ${UI.escapeHtml(patternName)}</span>
            </div>
            <div class="resource-url">
                <a href="${UI.escapeHtml(resource.link)}" target="_blank" rel="noopener noreferrer">
                    ${UI.escapeHtml(resource.link)}
                </a>
            </div>
            <div style="margin-top: 0.75rem; color: var(--text-muted); font-size: 0.9rem;">
                <div><strong>Produced:</strong> ${UI.escapeHtml(productionDate)}</div>
                <div><strong>Relevant Until:</strong> ${UI.escapeHtml(expirationDate)}</div>
            </div>
        `;
        container.appendChild(resourceItem);
    });
}
