// Clone roadmap modal state (needs to be global for displaySuggestedRoadmaps)
let selectedRoadmapForClone = null;
let currentStudyResources = [];
let isStudyingExpanded = false;
let currentNerves = [];
let isNervesExpanded = false;

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            try {
                await client.signOut();
            } catch (err) {
                console.error('Sign out error:', err);
            }
            localStorage.removeItem('token');
            window.location.href = '/index.html';
        });
    }

    // Roadmap modal handlers
    const createRoadmapBtn = document.getElementById('create-roadmap-btn');
    if (createRoadmapBtn) {
        createRoadmapBtn.addEventListener('click', () => {
            UI.toggleElement('roadmap-modal', true);
            UI.toggleElement('create-form-section', true);
            UI.toggleElement('clone-roadmap-section', false);
            const roadmapModalTitle = document.getElementById('roadmap-modal-title');
            if (roadmapModalTitle) {
                roadmapModalTitle.textContent = 'Create New Roadmap';
            }
            const roadmapName = document.getElementById('roadmap-name');
            if (roadmapName) {
                roadmapName.focus();
            }
            document.body.style.overflow = 'hidden';
        });
    }

    const closeRoadmapModal = () => {
        UI.toggleElement('roadmap-modal', false);
        UI.clearForm('roadmap-form');
        UI.toggleElement('roadmap-search-results', false);
        const suggestedList = document.getElementById('suggested-roadmaps-list');
        if (suggestedList) {
            suggestedList.innerHTML = '';
        }
        selectedRoadmapForClone = null;
        document.body.style.overflow = '';
    };

    // Close on backdrop click
    const roadmapModal = document.getElementById('roadmap-modal');
    if (roadmapModal) {
        roadmapModal.addEventListener('click', (e) => {
            if (e.target.id === 'roadmap-modal') {
                closeRoadmapModal();
            }
        });
    }

    // Close buttons and Back button
    const closeRoadmapModalBtn = document.getElementById('close-roadmap-modal-btn');
    if (closeRoadmapModalBtn) {
        closeRoadmapModalBtn.addEventListener('click', closeRoadmapModal);
    }

    const cancelRoadmapBtn = document.getElementById('cancel-roadmap-btn');
    if (cancelRoadmapBtn) {
        cancelRoadmapBtn.addEventListener('click', closeRoadmapModal);
    }

    const cancelCloneBtn = document.getElementById('cancel-clone-btn');
    if (cancelCloneBtn) {
        cancelCloneBtn.addEventListener('click', () => {
            UI.toggleElement('create-form-section', true);
            UI.toggleElement('clone-roadmap-section', false);
            const roadmapModalTitle = document.getElementById('roadmap-modal-title');
            if (roadmapModalTitle) {
                roadmapModalTitle.textContent = 'Create New Roadmap';
            }
            selectedRoadmapForClone = null;
        });
    }

    // Search roadmaps as user types
    let roadmapSearchTimeout;
    const roadmapNameInput = document.getElementById('roadmap-name');
    if (roadmapNameInput) {
        roadmapNameInput.addEventListener('input', async (e) => {
            const searchToken = e.target.value.trim();

            clearTimeout(roadmapSearchTimeout);

            if (searchToken.length < 1) {
                UI.toggleElement('roadmap-search-results', false);
                return;
            }

            roadmapSearchTimeout = setTimeout(async () => {
                try {
                    const roadmaps = await client.searchRoadmaps(searchToken);
                    displaySuggestedRoadmaps(roadmaps);
                } catch (err) {
                    console.error('Roadmap search error:', err);
                }
            }, 300);
        });
    }


    const confirmCloneBtn = document.getElementById('confirm-clone-btn');
    if (confirmCloneBtn) {
        confirmCloneBtn.addEventListener('click', async () => {
            if (!selectedRoadmapForClone) return;

            UI.setButtonLoading('confirm-clone-btn', true);

            try {
                const clonedRoadmap = await client.cloneRoadmap(selectedRoadmapForClone.id);

                closeRoadmapModal();
                UI.setButtonLoading('confirm-clone-btn', false);

                // Navigate to the cloned roadmap
                window.location.href = `roadmap.html?id=${clonedRoadmap.id}&name=${encodeURIComponent(clonedRoadmap.name)}`;
            } catch (err) {
                console.error('Clone roadmap failed:', err);
                UI.showError(err.message || 'Failed to clone roadmap');
                UI.setButtonLoading('confirm-clone-btn', false);
            }
        });
    }

    // Nerve modal handlers
    const createNerveBtn = document.getElementById('create-nerve-btn');
    if (createNerveBtn) {
        createNerveBtn.addEventListener('click', () => {
            UI.toggleElement('nerve-modal', true);
            const nerveSubject = document.getElementById('nerve-subject');
            if (nerveSubject) {
                nerveSubject.focus();
            }
            document.body.style.overflow = 'hidden';


        });
    }

    const closeNerveModal = () => {
        UI.toggleElement('nerve-modal', false);
        UI.clearForm('nerve-form');
        UI.toggleElement('nerve-subject-results', false);
        const nerveSubjectIdInput = document.getElementById('nerve-subject-id');
        if (nerveSubjectIdInput) {
            nerveSubjectIdInput.value = '';
        }
        UI.hideMessage('error-message');
        document.body.style.overflow = '';
    };

    const closeNerveModalBtn = document.getElementById('close-nerve-modal-btn');
    if (closeNerveModalBtn) {
        closeNerveModalBtn.addEventListener('click', closeNerveModal);
    }
    const cancelNerveBtn = document.getElementById('cancel-nerve-btn');
    if (cancelNerveBtn) {
        cancelNerveBtn.addEventListener('click', closeNerveModal);
    }

    // Close on backdrop click
    const nerveModal = document.getElementById('nerve-modal');
    if (nerveModal) {
        nerveModal.addEventListener('click', (e) => {
            if (e.target.id === 'nerve-modal') {
                closeNerveModal();
            }
        });
    }

    // Subject search for nerve creation
    let subjectSearchTimeout;
    const nerveSubjectInput = document.getElementById('nerve-subject');
    if (nerveSubjectInput) {
        nerveSubjectInput.addEventListener('input', async (e) => {
            const searchToken = e.target.value.trim();

            clearTimeout(subjectSearchTimeout);

            if (searchToken.length < 1) {
                UI.toggleElement('nerve-subject-results', false);
                return;
            }

            subjectSearchTimeout = setTimeout(async () => {
                try {
                    const subjects = await client.searchSubjects(searchToken);
                    displayNerveSubjectResults(subjects);
                } catch (err) {
                    console.error('Subject search error:', err);
                }
            }, 300);
        });
    }

    const nerveForm = document.getElementById('nerve-form');
    if (nerveForm) {
        nerveForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const subjectIdInput = document.getElementById('nerve-subject-id');
            const nameInput = document.getElementById('nerve-name');

            if (!subjectIdInput || !nameInput) return;

            const subjectId = subjectIdInput.value;
            const name = nameInput.value;
            const type = 8;
            const pattern = 6;

            if (!subjectId) {
                UI.showError('Please select a subject');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-nerve-btn', true);

            try {
                await client.createResource(parseInt(subjectId), name, type, pattern, '');

                closeNerveModal();
                UI.setButtonLoading('save-nerve-btn', false);

                loadNerves();
            } catch (err) {
                console.error('Create nerve failed:', err);
                UI.showError(err.message || 'Failed to create knowledge resource');
                UI.setButtonLoading('save-nerve-btn', false);
            }
        });
    }

    const roadmapForm = document.getElementById('roadmap-form');
    if (roadmapForm) {
        roadmapForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const nameInput = document.getElementById('roadmap-name');
            if (!nameInput) return;
            const name = UI.capitalizeWords(nameInput.value);

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-roadmap-btn', true);

            try {
                const roadmap = await client.createRoadmap(name);

                closeRoadmapModal();
                UI.setButtonLoading('save-roadmap-btn', false);

                loadRoadmaps();
            } catch (err) {
                console.error('Create roadmap failed:', err);
                UI.showError(err.message || 'Failed to create roadmap');
                UI.setButtonLoading('save-roadmap-btn', false);
            }
        });
    }
    
    loadRoadmaps();
    loadStudyingResources();
    loadNerves();
    
    // Search event listeners
    const studyingSearchInput = document.getElementById('studying-search-input');
    if (studyingSearchInput) {
        studyingSearchInput.addEventListener('input', (e) => {
            renderStudyingResources(currentStudyResources);
        });
    }

    const nervesSearchInput = document.getElementById('nerves-search-input');
    if (nervesSearchInput) {
        nervesSearchInput.addEventListener('input', (e) => {
            renderNerves(currentNerves);
        });
    }

    // Toggle buttons event listeners
    const studyingToggleBtn = document.getElementById('studying-toggle-btn');
    if (studyingToggleBtn) {
        studyingToggleBtn.addEventListener('click', () => {
            isStudyingExpanded = !isStudyingExpanded;
            renderStudyingResources(currentStudyResources);
        });
    }

    const nervesToggleBtn = document.getElementById('nerves-toggle-btn');
    if (nervesToggleBtn) {
        nervesToggleBtn.addEventListener('click', () => {
            isNervesExpanded = !isNervesExpanded;
            renderNerves(currentNerves);
        });
    }
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
    UI.toggleElement('studying-toggle-container', false);
    UI.toggleElement('studying-search-container', false);

    try {
        currentStudyResources = await client.getStudyResources();

        UI.toggleElement('studying-loading', false);

        if (currentStudyResources.length === 0) {
            UI.toggleElement('studying-empty-state', true);
            UI.toggleElement('studying-resources', false);
            UI.toggleElement('studying-search-container', false);
        } else {
            UI.toggleElement('studying-section', true);
            UI.toggleElement('studying-resources', true);
            UI.toggleElement('studying-search-container', true);
            UI.toggleElement('studying-empty-state', false);
            renderStudyingResources(currentStudyResources);
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
        const roadmapEl = document.createElement('a');
        roadmapEl.href = `roadmap.html?id=${roadmap.id}&name=${encodeURIComponent(roadmap.name)}`;
        roadmapEl.className = 'item-block compact';
        roadmapEl.style.textDecoration = 'none';

        roadmapEl.innerHTML = `
            <div class="item-header" style="margin-bottom: 0;">
                <h3 class="item-title" style="font-size: var(--font-size-base); word-break: break-word;">${UI.escapeHtml(roadmap.name)}</h3>
            </div>
        `;

        container.appendChild(roadmapEl);
    });
}

function renderStudyingResources(resources) {
    const container = document.getElementById('studying-resources');
    const toggleContainer = document.getElementById('studying-toggle-container');
    const toggleBtn = document.getElementById('studying-toggle-btn');
    const searchInput = document.getElementById('studying-search-input');
    const searchTerm = searchInput ? searchInput.value.toLowerCase().trim() : '';
    container.innerHTML = '';

    const typeNames = ['Book', 'Website', 'Course', 'Video', 'Channel', 'Mailing List', 'Manual', 'Slides', 'Your Knowledge'];
    const patternNames = ['Chapters', 'Pages', 'Sessions', 'Episodes', 'Playlist', 'Posts', 'Memories'];

    // Filter by search term
    let filteredResources = resources;
    if (searchTerm) {
        filteredResources = resources.filter(resource => 
            resource.name.toLowerCase().includes(searchTerm) ||
            (typeNames[resource.type] || '').toLowerCase().includes(searchTerm) ||
            (patternNames[resource.pattern] || '').toLowerCase().includes(searchTerm) ||
            (resource.milestone.name || '').toLowerCase().includes(searchTerm)
        );
    }

    // Sort by order
    const sortedResources = filteredResources.sort((a, b) => (a.order || 0) - (b.order || 0));

    // Determine resources to show
    const resourcesToShow = isStudyingExpanded ? sortedResources : sortedResources.slice(0, 3);

    // Update toggle button visibility and text
    if (sortedResources.length > 3) {
        UI.toggleElement('studying-toggle-container', true);
        if (toggleBtn) toggleBtn.textContent = isStudyingExpanded ? 'Show Less' : `Show All (${sortedResources.length})`;
    } else {
        UI.toggleElement('studying-toggle-container', false);
    }

    resourcesToShow.forEach(resource => {
        const resourceItem = document.createElement('div');
        resourceItem.className = 'item-block compact';
        resourceItem.style.cursor = 'pointer';

        const typeName = typeNames[resource.type] || 'Unknown';
        const patternName = patternNames[resource.pattern] || 'Unknown';

        const subjectId = resource.milestone.id || '';
        const subjectName = resource.milestone.name || '';
        const level = resource.milestone.level || '';

        const resProviders = resource.providers || [];
        const resPresenters = resource.presenters || [];
        const resMetaHtml = (resProviders.length > 0 || resPresenters.length > 0) ? `
            <div style="display: flex; flex-wrap: wrap; gap: 0.75rem; font-size: 0.72rem; color: var(--color-text-muted); margin-top: 0.15rem; pointer-events: none;">
                ${resProviders.length > 0 ? `<span>from <span style="color: var(--color-text);">${resProviders.map(p => UI.escapeHtml(p.name)).join(', ')}</span></span>` : ''}
                ${resPresenters.length > 0 ? `<span>by <span style="color: var(--color-text);">${resPresenters.map(p => UI.escapeHtml(p.name)).join(', ')}</span></span>` : ''}
            </div>` : '';

        resourceItem.innerHTML = `
            <div style="width: 100%; display: flex; flex-direction: column; gap: 0.25rem; pointer-events: none;">
                <div class="item-header" style="margin-bottom: 0; align-items: flex-start; flex-wrap: wrap; gap: var(--space-xs); pointer-events: auto;">
                    <div style="display: flex; align-items: flex-start; gap: var(--space-xs); flex: 1; min-width: 180px; pointer-events: none;">
                        <span class="resource-icon">${UI.getResourceIcon(resource.type)}</span>
                        <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); overflow-wrap: break-word; word-break: break-word;">${UI.escapeHtml(resource.name)}</h3>
                    </div>
                    <div style="display: flex; gap: var(--space-xs); align-items: center; flex-shrink: 0; margin-left: auto; pointer-events: auto;">
                        <span class="item-badge" style="font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; pointer-events: none;">${UI.escapeHtml(typeName)}</span>
                        <span class="item-badge" style="background: rgba(102, 126, 234, 0.2); color: var(--color-primary-start); font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; pointer-events: none;">${UI.escapeHtml(patternName)}</span>
                        ${resource.link ? `<a href="${UI.escapeHtml(resource.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()" class="external-link-icon" title="Open Link" style="display: flex; align-items: center; justify-content: center; width: 28px; height: 28px; border-radius: 50%; background: rgba(255,255,255,0.05); transition: background 0.2s;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path><polyline points="15 3 21 3 21 9"></polyline><line x1="10" y1="14" x2="21" y2="3"></line></svg>
                        </a>` : ''}
                    </div>
                </div>
                ${resMetaHtml}
            </div>
        `;

        resourceItem.addEventListener('click', () => {
            UI.showPageLoading();
            window.location.href = `resource.html?id=${resource.id}&name=${encodeURIComponent(resource.name)}&type=${resource.type}&pattern=${resource.pattern}&link=${encodeURIComponent(resource.link)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&level=${level}`;
        });

        container.appendChild(resourceItem);
    });
}

async function loadNerves() {
    UI.toggleElement('nerves-loading', true);
    UI.toggleElement('nerves-resources', false);
    UI.toggleElement('nerves-empty-state', false);
    UI.toggleElement('nerves-toggle-container', false);
    UI.toggleElement('nerves-search-container', false);

    try {
        currentNerves = await client.getNerves();

        UI.toggleElement('nerves-loading', false);

        if (currentNerves.length === 0) {
            UI.toggleElement('nerves-empty-state', true);
            UI.toggleElement('nerves-resources', false);
            UI.toggleElement('nerves-search-container', false);
        } else {
            UI.toggleElement('nerves-resources', true);
            UI.toggleElement('nerves-search-container', true);
            UI.toggleElement('nerves-empty-state', false);
            renderNerves(currentNerves);
        }
    } catch (err) {
        console.error('Load nerves failed:', err);
        UI.toggleElement('nerves-loading', false);
        // Don't hide the whole section, just show empty state or silent fail
        UI.toggleElement('nerves-empty-state', true);
    }
}

function renderNerves(nerves) {
    const container = document.getElementById('nerves-resources');
    const toggleContainer = document.getElementById('nerves-toggle-container');
    const toggleBtn = document.getElementById('nerves-toggle-btn');
    const searchInput = document.getElementById('nerves-search-input');
    const searchTerm = searchInput ? searchInput.value.toLowerCase().trim() : '';
    container.innerHTML = '';

    // Filter by search term
    let filteredNerves = nerves;
    if (searchTerm) {
        filteredNerves = nerves.filter(nerve => 
            nerve.name.toLowerCase().includes(searchTerm) ||
            (nerve.milestone ? nerve.milestone.name : '').toLowerCase().includes(searchTerm)
        );
    }

    // Determine nerves to show
    const nervesToShow = isNervesExpanded ? filteredNerves : filteredNerves.slice(0, 3);

    // Update toggle button visibility and text
    if (filteredNerves.length > 3) {
        UI.toggleElement('nerves-toggle-container', true);
        if (toggleBtn) toggleBtn.textContent = isNervesExpanded ? 'Show Less' : `Show All (${filteredNerves.length})`;
    } else {
        UI.toggleElement('nerves-toggle-container', false);
    }

    nervesToShow.forEach(nerve => {
        const nerveItem = document.createElement('div');
        nerveItem.className = 'item-block compact';
        nerveItem.style.cursor = 'pointer';

        const typeName = 'Knowledge';
        const patternName = 'Memories';

        const subjectId = nerve.milestone ? nerve.milestone.id : '';
        const subjectName = nerve.milestone ? nerve.milestone.name : '';
        const level = nerve.milestone ? nerve.milestone.level : '';

        const subjectBadgeHtml = subjectId ? `
            <span class="item-badge" style="background: rgba(76, 175, 80, 0.1); color: #388e3c; font-size: 10px; height: 18px; min-width: auto; padding: 0 6px;">
                ${UI.escapeHtml(subjectName)}
            </span>
        ` : '';

        const nerveProviders = nerve.providers || [];
        const nervePresenters = nerve.presenters || [];
        const nerveMetaHtml = (nerveProviders.length > 0 || nervePresenters.length > 0) ? `
            <div style="display: flex; flex-wrap: wrap; gap: 0.75rem; font-size: 0.72rem; color: var(--color-text-muted); margin-top: 0.15rem; pointer-events: none;">
                ${nerveProviders.length > 0 ? `<span>from <span style="color: var(--color-text);">${nerveProviders.map(p => UI.escapeHtml(p.name)).join(', ')}</span></span>` : ''}
                ${nervePresenters.length > 0 ? `<span>by <span style="color: var(--color-text);">${nervePresenters.map(p => UI.escapeHtml(p.name)).join(', ')}</span></span>` : ''}
            </div>` : '';

        nerveItem.innerHTML = `
            <div style="width: 100%; display: flex; flex-direction: column; gap: 0.25rem; pointer-events: none;">
                <div class="item-header" style="margin-bottom: 0; align-items: flex-start; flex-wrap: wrap; gap: var(--space-xs); pointer-events: auto;">
                    <div style="display: flex; align-items: flex-start; gap: var(--space-xs); flex: 1; min-width: 180px; pointer-events: none;">
                        <span class="resource-icon">${UI.getResourceIcon(nerve.type)}</span>
                        <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); overflow-wrap: break-word; word-break: break-word;">${UI.escapeHtml(nerve.name)}</h3>
                    </div>
                    <div style="display: flex; gap: var(--space-xs); align-items: center; flex-shrink: 0; margin-left: auto; pointer-events: auto;">
                        ${subjectBadgeHtml}
                        <span class="item-badge" style="font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; pointer-events: none;">${UI.escapeHtml(typeName)}</span>
                        <span class="item-badge" style="background: rgba(102, 126, 234, 0.2); color: var(--color-primary-start); font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; pointer-events: none;">${UI.escapeHtml(patternName)}</span>
                        ${nerve.link ? `<a href="${UI.escapeHtml(nerve.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()" class="external-link-icon" title="Open Link" style="display: flex; align-items: center; justify-content: center; width: 28px; height: 28px; border-radius: 50%; background: rgba(255,255,255,0.05); transition: background 0.2s;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path><polyline points="15 3 21 3 21 9"></polyline><line x1="10" y1="14" x2="21" y2="3"></line></svg>
                        </a>` : ''}
                    </div>
                </div>
                ${nerveMetaHtml}
            </div>
        `;

        nerveItem.addEventListener('click', () => {
            UI.showPageLoading();
            window.location.href = `resource.html?id=${nerve.id}&name=${encodeURIComponent(nerve.name)}&type=${nerve.type}&pattern=${nerve.pattern}&link=${encodeURIComponent(nerve.link)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName)}&level=${level}`;
        });

        container.appendChild(nerveItem);
    });
}

function displayNerveSubjectResults(subjects) {
    const resultsContainer = document.getElementById('nerve-subject-results');

    if (!subjects || subjects.length === 0) {
        UI.toggleElement('nerve-subject-results', false);
        return;
    }

    resultsContainer.innerHTML = '';
    subjects.forEach(subject => {
        const resultItem = document.createElement('div');
        resultItem.className = 'search-result-item';
        resultItem.textContent = subject.name;
        resultItem.style.cursor = 'pointer';
        resultItem.style.padding = '0.75rem';
        resultItem.style.borderBottom = '1px solid var(--border-color)';

        resultItem.addEventListener('click', () => {
            const nerveSubjectInput = document.getElementById('nerve-subject');
            if (nerveSubjectInput) {
                nerveSubjectInput.value = subject.name;
            }
            const nerveSubjectIdInput = document.getElementById('nerve-subject-id');
            if (nerveSubjectIdInput) {
                nerveSubjectIdInput.value = subject.id;
            }
            UI.toggleElement('nerve-subject-results', false);
        });

        resultItem.addEventListener('mouseenter', () => {
            resultItem.style.backgroundColor = 'var(--bg-secondary)';
        });

        resultItem.addEventListener('mouseleave', () => {
            resultItem.style.backgroundColor = '';
        });

        resultsContainer.appendChild(resultItem);
    });

    UI.toggleElement('nerve-subject-results', true);
}

function displaySuggestedRoadmaps(roadmaps) {
    const listContainer = document.getElementById('suggested-roadmaps-list');

    if (!roadmaps || roadmaps.length === 0) {
        UI.toggleElement('roadmap-search-results', false);
        return;
    }

    listContainer.innerHTML = '';
    roadmaps.forEach(roadmap => {
        const roadmapItem = document.createElement('div');
        roadmapItem.style.cssText = 'padding: 0.75rem; margin-bottom: 0.5rem; background: var(--bg-primary); border: 1px solid var(--border-color); border-radius: 4px; cursor: pointer; transition: all 0.2s;';

        roadmapItem.innerHTML = `
            <span style="font-weight: 500; color: var(--text-primary);">${UI.escapeHtml(roadmap.name)}</span>
        `;

        roadmapItem.addEventListener('click', () => {
            selectedRoadmapForClone = roadmap;
            const cloneRoadmapName = document.getElementById('clone-roadmap-name');
            if (cloneRoadmapName) {
                cloneRoadmapName.textContent = roadmap.name;
            }
            UI.toggleElement('create-form-section', false);
            UI.toggleElement('clone-roadmap-section', true);
            const roadmapModalTitle = document.getElementById('roadmap-modal-title');
            if (roadmapModalTitle) {
                roadmapModalTitle.textContent = 'Clone Roadmap';
            }
        });

        roadmapItem.addEventListener('mouseenter', () => {
            roadmapItem.style.borderColor = 'var(--accent-color)';
            roadmapItem.style.transform = 'translateX(4px)';
        });

        roadmapItem.addEventListener('mouseleave', () => {
            roadmapItem.style.borderColor = 'var(--border-color)';
            roadmapItem.style.transform = 'translateX(0)';
        });

        listContainer.appendChild(roadmapItem);
    });

    UI.toggleElement('roadmap-search-results', true);
}
