window.addEventListener('DOMContentLoaded', () => {
    if (!Auth.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }
    
    document.getElementById('signout-btn').addEventListener('click', async (e) => {
        e.preventDefault();
        await Auth.signOut();
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
    
    // Handle roadmap creation
    document.getElementById('roadmap-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        
        const name = document.getElementById('roadmap-name').value;
        const description = document.getElementById('roadmap-description').value;
        
        UI.hideMessage('error-message');
        UI.setButtonLoading('save-roadmap-btn', true);
        
        try {
            const roadmap = await flashbackClient.createRoadmap(name, description);
            console.log('Roadmap created:', roadmap);
            
            // Hide form and reload roadmaps
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
});

async function loadRoadmaps() {
    UI.toggleElement('loading', true);
    UI.toggleElement('roadmaps-container', false);
    UI.toggleElement('empty-state', false);
    
    try {
        const roadmaps = await flashbackClient.getRoadmaps();

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
