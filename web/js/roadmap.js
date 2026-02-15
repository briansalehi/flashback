window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
    }

    const roadmapId = UI.getUrlParam('id');
    const roadmapName = UI.getUrlParam('name');
    if (!roadmapId) {
        window.location.href = '/home.html';
    }

    document.getElementById('signout-btn').addEventListener('click', async (e) => {
        e.preventDefault();
        await client.signOut();
        localStorage.removeItem('token');
        window.location.href = '/index.html';
    });

    document.getElementById('create-milestone-btn').addEventListener('click', () => {
        UI.toggleElement('add-milestone-form', true);
        UI.toggleElement('subject-search-input', true);
        document.getElementById('subject-search-input').focus();
    });

    document.getElementById('cancel-milestone-btn').addEventListener('click', () => {
        UI.toggleElement('add-milestone-form', false);
        UI.clearForm('milestone-form');
        clearSearchResults();
    });

    document.getElementById('create-new-subject-btn').addEventListener('click', () => {
        UI.toggleElement('create-subject-section', true);
        UI.toggleElement('search-subject-section', false);
        document.getElementById('new-subject-name').focus();
    });

    document.getElementById('back-to-search-btn').addEventListener('click', () => {
        UI.toggleElement('create-subject-section', false);
        UI.toggleElement('search-subject-section', true);
        UI.clearForm('create-subject-form');
    });

    // Subject search with debounce
    let searchTimeout;
    document.getElementById('subject-search-input').addEventListener('input', (e) => {
        clearTimeout(searchTimeout);
        const searchToken = e.target.value.trim();

        if (searchToken.length < 2) {
            clearSearchResults();
            return;
        }

        searchTimeout = setTimeout(async () => {
            await searchSubjects(searchToken);
        }, 300);
    });

    document.getElementById('create-subject-form').addEventListener('submit', async (e) => {
        e.preventDefault();

        const name = document.getElementById('new-subject-name').value;

        UI.hideMessage('error-message');
        UI.setButtonLoading('save-new-subject-btn', true);

        try {
            await client.createSubject(name);

            // After creating, search for it to add to roadmap
            await searchSubjects(name);

            UI.toggleElement('create-subject-section', false);
            UI.toggleElement('search-subject-section', true);
            UI.clearForm('create-subject-form');
            UI.setButtonLoading('save-new-subject-btn', false);

            UI.showMessage('Subject created! Select it below to add to your roadmap.', 'success');
        } catch (err) {
            console.error('Create subject failed:', err);
            UI.showError(err.message || 'Failed to create subject');
            UI.setButtonLoading('save-new-subject-btn', false);
        }
    });

    document.getElementById('milestone-form').addEventListener('submit', async (e) => {
        e.preventDefault();

        const selectedSubject = document.querySelector('input[name="subject-select"]:checked');
        if (!selectedSubject) {
            UI.showError('Please select a subject');
            return;
        }

        const subjectId = selectedSubject.value;
        const level = document.getElementById('expertise-level').value;

        UI.hideMessage('error-message');
        UI.setButtonLoading('save-milestone-btn', true);

        try {
            await client.addMilestone(roadmapId, subjectId, parseInt(level));

            UI.toggleElement('add-milestone-form', false);
            UI.clearForm('milestone-form');
            clearSearchResults();
            UI.setButtonLoading('save-milestone-btn', false);

            loadMilestones();
        } catch (err) {
            console.error('Add milestone failed:', err);
            UI.showError(err.message || 'Failed to add subject to roadmap');
            UI.setButtonLoading('save-milestone-btn', false);
        }
    });

    loadMilestones(roadmapId, roadmapName);
});

async function searchSubjects(searchToken) {
    UI.toggleElement('search-results-loading', true);
    UI.toggleElement('search-results', false);
    UI.toggleElement('no-search-results', false);

    try {
        const results = await client.searchSubjects(searchToken);

        UI.toggleElement('search-results-loading', false);

        if (results.length === 0) {
            UI.toggleElement('no-search-results', true);
        } else {
            UI.toggleElement('search-results', true);
            renderSearchResults(results);
        }
    } catch (err) {
        console.error('Search subjects failed:', err);
        UI.toggleElement('search-results-loading', false);
        UI.showError('Failed to search subjects: ' + (err.message || 'Unknown error'));
    }
}

function renderSearchResults(subjects) {
    const container = document.getElementById('search-results');
    container.innerHTML = '';

    subjects.forEach((subject, index) => {
        const resultItem = document.createElement('div');
        resultItem.className = 'search-result-item';
        resultItem.innerHTML = `
            <label class="search-result-label">
                <input 
                    type="radio" 
                    name="subject-select" 
                    value="${subject.id}"
                    ${index === 0 ? 'checked' : ''}
                >
                <div>
                    <div class="search-result-name">${UI.escapeHtml(subject.name)}</div>
                </div>
            </label>
        `;
        container.appendChild(resultItem);
    });
}

function clearSearchResults() {
    document.getElementById('subject-search-input').value = '';
    document.getElementById('search-results').innerHTML = '';
    UI.toggleElement('search-results', false);
    UI.toggleElement('no-search-results', false);
    UI.toggleElement('search-results-loading', false);
}

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
