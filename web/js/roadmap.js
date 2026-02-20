window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const roadmapId = UI.getUrlParam('id');
    const roadmapName = UI.getUrlParam('name');
    if (!roadmapId) {
        window.location.href = '/home.html';
        return;
    }

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            localStorage.removeItem('token');
            window.location.href = '/index.html';
        });
    }

    const createMilestoneBtn = document.getElementById('create-milestone-btn');
    if (createMilestoneBtn) {
        createMilestoneBtn.addEventListener('click', (e) => {
            e.preventDefault();

            // Show the form
            UI.toggleElement('add-milestone-form', true);
            
            // Make sure search section is visible and create section is hidden
            UI.toggleElement('search-subject-section', true);
            UI.toggleElement('create-subject-section', false);
            
            // Use setTimeout to ensure the element is visible before focusing
            setTimeout(() => {
                const searchInput = document.getElementById('subject-search-input');
                if (searchInput) {
                    searchInput.disabled = false;
                    searchInput.focus();
                }
            }, 100);
        });
    } else {
        console.error('Create milestone button not found!');
    }

    const cancelMilestoneBtn = document.getElementById('cancel-milestone-btn');
    if (cancelMilestoneBtn) {
        cancelMilestoneBtn.addEventListener('click', () => {
            UI.toggleElement('add-milestone-form', false);
            UI.clearForm('milestone-form');
            clearSearchResults();
        });
    }

    const createNewSubjectBtn = document.getElementById('create-new-subject-btn');
    if (createNewSubjectBtn) {
        createNewSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('create-subject-section', true);
            UI.toggleElement('search-subject-section', false);
            setTimeout(() => {
                const nameInput = document.getElementById('new-subject-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const backToSearchBtn = document.getElementById('back-to-search-btn');
    if (backToSearchBtn) {
        backToSearchBtn.addEventListener('click', () => {
            UI.toggleElement('create-subject-section', false);
            UI.toggleElement('search-subject-section', true);
            UI.clearForm('create-subject-form');
        });
    }

    let searchTimeout;
    const subjectSearchInput = document.getElementById('subject-search-input');
    if (subjectSearchInput) {
        subjectSearchInput.addEventListener('input', (e) => {
            clearTimeout(searchTimeout);
            const searchToken = e.target.value.trim();

            searchTimeout = setTimeout(async () => {
                await searchSubjects(searchToken);
            }, 300);
        });
    } else {
        console.error('Search input not found!');
    }

    const saveNewSubjectBtn = document.getElementById('save-new-subject-btn');
    if (saveNewSubjectBtn) {
        saveNewSubjectBtn.addEventListener('click', async () => {
            const nameInput = document.getElementById('new-subject-name');
            const name = nameInput.value.trim();

            if (!name) {
                UI.showError('Please enter a subject name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-new-subject-btn', true);

            try {
                await client.createSubject(name);

                // After creating, search for it to add to roadmap
                await searchSubjects(name);

                UI.toggleElement('create-subject-section', false);
                UI.toggleElement('search-subject-section', true);
                nameInput.value = '';
                UI.setButtonLoading('save-new-subject-btn', false);

                UI.showSuccess('Subject created! Select it below to add to your roadmap.');
            } catch (err) {
                console.error('Create subject failed:', err);
                UI.showError(err.message || 'Failed to create subject');
                UI.setButtonLoading('save-new-subject-btn', false);
            }
        });
    }

    const milestoneForm = document.getElementById('milestone-form');
    if (milestoneForm) {
        milestoneForm.addEventListener('submit', async (e) => {
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
    }

    // Rename roadmap handlers
    const renameRoadmapBtn = document.getElementById('rename-roadmap-btn');
    if (renameRoadmapBtn) {
        renameRoadmapBtn.addEventListener('click', () => {
            const modal = document.getElementById('rename-roadmap-modal');
            UI.toggleElement('rename-roadmap-modal', true);
            document.getElementById('rename-roadmap-name').value = roadmapName || '';
            setTimeout(() => {
                modal.scrollIntoView({ behavior: 'smooth', block: 'start' });
                document.getElementById('rename-roadmap-name').focus();
            }, 100);
        });
    }

    const cancelRenameBtn = document.getElementById('cancel-rename-btn');
    if (cancelRenameBtn) {
        cancelRenameBtn.addEventListener('click', () => {
            UI.toggleElement('rename-roadmap-modal', false);
            UI.clearForm('rename-roadmap-form');
        });
    }

    const renameRoadmapForm = document.getElementById('rename-roadmap-form');
    if (renameRoadmapForm) {
        renameRoadmapForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const newName = document.getElementById('rename-roadmap-name').value.trim();
            if (!newName) {
                UI.showError('Please enter a roadmap name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-rename-btn', true);

            try {
                await client.renameRoadmap(roadmapId, newName);

                UI.toggleElement('rename-roadmap-modal', false);
                UI.clearForm('rename-roadmap-form');
                UI.setButtonLoading('save-rename-btn', false);

                // Update the URL and page
                const newUrl = `roadmap.html?id=${roadmapId}&name=${encodeURIComponent(newName)}`;
                window.history.replaceState({}, '', newUrl);
                document.getElementById('roadmap-name').textContent = newName;
                document.title = `${newName} - Flashback`;

                UI.showSuccess('Roadmap renamed successfully');
            } catch (err) {
                console.error('Rename roadmap failed:', err);
                UI.showError(err.message || 'Failed to rename roadmap');
                UI.setButtonLoading('save-rename-btn', false);
            }
        });
    }

    // Remove roadmap handlers
    const removeRoadmapBtn = document.getElementById('remove-roadmap-btn');
    if (removeRoadmapBtn) {
        removeRoadmapBtn.addEventListener('click', () => {
            const modal = document.getElementById('remove-roadmap-modal');
            UI.toggleElement('remove-roadmap-modal', true);
            setTimeout(() => {
                modal.scrollIntoView({ behavior: 'smooth', block: 'start' });
            }, 100);
        });
    }

    const cancelRemoveBtn = document.getElementById('cancel-remove-btn');
    if (cancelRemoveBtn) {
        cancelRemoveBtn.addEventListener('click', () => {
            UI.toggleElement('remove-roadmap-modal', false);
        });
    }

    const confirmRemoveBtn = document.getElementById('confirm-remove-btn');
    if (confirmRemoveBtn) {
        confirmRemoveBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-btn', true);

            try {
                await client.removeRoadmap(roadmapId);

                UI.toggleElement('remove-roadmap-modal', false);
                UI.setButtonLoading('confirm-remove-btn', false);

                // Redirect to home page
                window.location.href = '/home.html';
            } catch (err) {
                console.error('Remove roadmap failed:', err);
                UI.showError(err.message || 'Failed to remove roadmap');
                UI.setButtonLoading('confirm-remove-btn', false);
            }
        });
    }

    loadMilestones();
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
        document.title = `${roadmapName || 'Roadmap'} - Flashback`;

        // Show rename and remove buttons
        UI.toggleElement('rename-roadmap-btn', true);
        UI.toggleElement('remove-roadmap-btn', true);

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

    const levelNames = ['Surface', 'Depth', 'Origin'];

    milestones.forEach((milestone, index) => {
        const levelName = levelNames[milestone.level] || 'Unknown';

        const milestoneCard = document.createElement('div');
        milestoneCard.className = 'milestone-card';
        milestoneCard.draggable = true;
        milestoneCard.dataset.position = milestone.position;
        milestoneCard.innerHTML = `
            <div class="milestone-header">
                <div class="milestone-position">${index + 1}</div>
                <h3 class="milestone-name">${UI.escapeHtml(milestone.name)}</h3>
                <span class="milestone-level">${UI.escapeHtml(levelName)}</span>
            </div>
            <button class="remove-milestone-btn" data-id="${milestone.id}" title="Remove milestone">×</button>
        `;

        // Click to navigate (but not when dragging)
        let isDragging = false;
        milestoneCard.addEventListener('click', () => {
            if (!isDragging) {
                const roadmapId = UI.getUrlParam('id');
                const roadmapName = UI.getUrlParam('name');
                window.location.href = `subject.html?id=${milestone.id}&name=${encodeURIComponent(milestone.name)}&level=${milestone.level}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
            }
        });

        // Drag and drop handlers
        milestoneCard.addEventListener('dragstart', (e) => {
            isDragging = true;
            milestoneCard.classList.add('dragging');
            e.dataTransfer.effectAllowed = 'move';
            e.dataTransfer.setData('text/plain', milestone.position);
        });

        milestoneCard.addEventListener('dragend', () => {
            milestoneCard.classList.remove('dragging');
            setTimeout(() => {
                isDragging = false;
            }, 100);
        });

        milestoneCard.addEventListener('dragover', (e) => {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'move';

            const draggingCard = document.querySelector('.dragging');
            if (draggingCard && draggingCard !== milestoneCard) {
                milestoneCard.classList.add('drag-over');
            }
        });

        milestoneCard.addEventListener('dragleave', () => {
            milestoneCard.classList.remove('drag-over');
        });

        milestoneCard.addEventListener('drop', async (e) => {
            e.preventDefault();
            milestoneCard.classList.remove('drag-over');

            const currentPosition = parseInt(e.dataTransfer.getData('text/plain'));
            const targetPosition = parseInt(milestone.position);

            if (currentPosition !== targetPosition) {
                await reorderMilestone(currentPosition, targetPosition);
            }
        });

        // Remove milestone handler
        const removeBtn = milestoneCard.querySelector('.remove-milestone-btn');
        if (removeBtn) {
            removeBtn.addEventListener('click', async (e) => {
                e.stopPropagation(); // Prevent card click navigation

                if (confirm(`Are you sure you want to remove "${milestone.name}" from this roadmap?`)) {
                    await removeMilestone(milestone.id);
                }
            });
        }

        container.appendChild(milestoneCard);
    });
}

async function reorderMilestone(currentPosition, targetPosition) {
    const roadmapId = UI.getUrlParam('id');

    try {
        await client.reorderMilestone(roadmapId, currentPosition, targetPosition);
        await loadMilestones();
    } catch (err) {
        console.error('Reorder milestone failed:', err);
        UI.showError('Failed to reorder milestone: ' + (err.message || 'Unknown error'));
    }
}

async function removeMilestone(milestoneId) {
    const roadmapId = UI.getUrlParam('id');

    try {
        await client.removeMilestone(roadmapId, milestoneId);
        await loadMilestones();
        UI.showSuccess('Milestone removed successfully');
    } catch (err) {
        console.error('Remove milestone failed:', err);
        UI.showError('Failed to remove milestone: ' + (err.message || 'Unknown error'));
    }
}
