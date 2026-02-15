window.addEventListener('DOMContentLoaded', () => {
    if (!Auth.isAuthenticated()) {
        window.location.href = '/index.html';
    }

    const roadmapId = UI.getUrlParam('id');
    const roadmapName = UI.getUrlParam('name');
    if (!roadmapId) {
        window.location.href = '/home.html';
    }

    document.getElementById('signout-btn').addEventListener('click', async (e) => {
        e.preventDefault();
        await Auth.signOut();
    });

    document.getElementById('create-milestone-btn').addEventListener('click', () => {
        UI.toggleElement('create-form', true);
        document.getElementById('milestone-name').focus();
    });

    document.getElementById('cancel-milestone-btn').addEventListener('click', () => {
        UI.toggleElement('create-form', false);
        UI.clearForm('milestone-form');
    });

    document.getElementById('milestone-form').addEventListener('submit', async (e) => {
        e.preventDefault();

        const name = document.getElementById('milestone-name').value;
        const description = document.getElementById('milestone-description').value;
        const dateStr = document.getElementById('milestone-date').value;
        const targetDate = dateStr ? new Date(dateStr) : null;

        UI.hideMessage('error-message');
        UI.setButtonLoading('save-milestone-btn', true);

        try {
            const milestone = await client.createMilestone(
                roadmapId,
                name,
                description,
                targetDate
            );
            console.log('Milestone created:', milestone);

            UI.toggleElement('create-form', false);
            UI.clearForm('milestone-form');
            UI.setButtonLoading('save-milestone-btn', false);

            loadMilestones();
        } catch (err) {
            console.error('Create milestone failed:', err);
            UI.showError(err.message || 'Failed to create milestone');
            UI.setButtonLoading('save-milestone-btn', false);
        }
    });

    loadMilestones(roadmapId, roadmapName);
});

async function loadMilestones() {
    UI.toggleElement('loading', true);
    UI.toggleElement('milestones-container', false);
    UI.toggleElement('empty-state', false);

    try {
        const roadmapId = UI.getUrlParam('id');
        const roadmapName = UI.getUrlParam('name');
        const response = await client.getMilestones(roadmapId);

        document.getElementById('roadmap-name').textContent = roadmapName;
        document.title = roadmapName;

        UI.toggleElement('loading', false);

        if (response.milestones.length === 0) {
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('milestones-container', true);
            renderMilestones(response.milestones);
        }
    } catch (err) {
        console.error('Loading milestones failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load milestones: ' + (err.message || 'Unknown error'));
    }
}

function renderMilestones(milestones) {
    const container = document.getElementById('milestones-container');
    container.innerHTML = '';

    milestones.forEach(milestone => {
        container.innerHTML += `
            <div class="card-title">
                <a href="subject.html?id=${milestone.id}" class="btn btn-secondary mt-md">${UI.escapeHtml(milestone.name)} (${UI.escapeHtml(milestone.level)})</a>
            </div>
        `;
    });
}