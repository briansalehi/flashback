window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }
    
    document.getElementById('signout-btn').addEventListener('click', async (e) => {
        e.preventDefault();
        try {
            await client.signOut();
        } catch (err) {
            console.error('Sign out error:', err);
        }
        localStorage.removeItem('token');
        window.location.href = '/index.html';
    });
    
    // Show/hide create form
    document.getElementById('create-roadmap-btn').addEventListener('click', () => {
        UI.toggleElement('create-form', true);
        document.getElementById('roadmap-name').focus();
    });
    
    document.getElementById('cancel-roadmap-btn').addEventListener('click', () => {
        UI.toggleElement('create-form', false);
        UI.clearForm('roadmap-form');
    });
    
    document.getElementById('roadmap-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const name = document.getElementById('roadmap-name').value;

        UI.hideMessage('error-message');
        UI.setButtonLoading('save-roadmap-btn', true);
        
        try {
            const roadmap = await client.createRoadmap(name);

            UI.toggleElement('create-form', false);
            UI.clearForm('roadmap-form');
            UI.setButtonLoading('save-roadmap-btn', false);
            
            loadRoadmaps();
        } catch (err) {
            console.error('Create roadmap failed:', err);
            UI.showError(err.message || 'Failed to create roadmap');
            UI.setButtonLoading('save-roadmap-btn', false);
        }
    });
    
    loadRoadmaps();
    loadStudyingResources();
});

async function loadRoadmaps() {
    UI.toggleElement('loading', true);
    UI.toggleElement('roadmaps-container', false);
    UI.toggleElement('empty-state', false);

    try {
        const roadmaps = await client.getRoadmaps();

        UI.toggleElement('loading', false);

        if (roadmaps.length === 0) {
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('roadmaps-container', true);
            renderRoadmaps(roadmaps);
        }
    } catch (err) {
        console.error('Load roadmaps failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load roadmaps: ' + (err.message || 'Unknown error'));
    }
}

async function loadStudyingResources() {
    UI.toggleElement('studying-loading', true);
    UI.toggleElement('studying-resources', false);
    UI.toggleElement('studying-empty-state', false);

    try {
        const studyResources = await client.getStudyResources();

        UI.toggleElement('studying-loading', false);

        if (studyResources.length === 0) {
            UI.toggleElement('studying-empty-state', true);
        } else {
            UI.toggleElement('studying-section', true);
            UI.toggleElement('studying-resources', true);
            renderStudyingResources(studyResources);
        }
    } catch (err) {
        console.error('Load studying resources failed:', err);
        UI.toggleElement('studying-loading', false);
        // Don't show error for studying resources, just hide the section
    }
}

function renderRoadmaps(roadmaps) {
    const container = document.getElementById('roadmaps-container');
    container.innerHTML = '';

    roadmaps.forEach(roadmap => {
        container.innerHTML += `
            <a href="roadmap.html?id=${roadmap.id}&name=${UI.escapeHtml(roadmap.name)}" class="roadmap-card">
                <h3 class="roadmap-title">${UI.escapeHtml(roadmap.name)}</h3>
            </a>
        `;
    });
}

function renderStudyingResources(resources) {
    const container = document.getElementById('studying-resources');
    container.innerHTML = '';

    // Sort by position (order)
    const sortedResources = resources.sort((a, b) => (a.position || 0) - (b.position || 0));

    sortedResources.forEach(resource => {
        const card = document.createElement('a');
        card.className = 'roadmap-card';
        card.style.cursor = 'pointer';

        if (resource.link) {
            card.href = resource.link;
            card.target = '_blank';
            card.rel = 'noopener noreferrer';
        } else {
            card.addEventListener('click', (e) => {
                e.preventDefault();
            });
        }

        card.innerHTML = `
            <h3 class="roadmap-title">${UI.escapeHtml(resource.name)}</h3>
        `;

        container.appendChild(card);
    });
}
