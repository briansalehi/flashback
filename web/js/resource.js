// Store current resource data
let currentResourceData = null;
let resourceId = UI.getUrlParam('id');
let resourceName = UI.getUrlParam('name') || '';
let subjectId = UI.getUrlParam('subjectId');
let roadmapId = UI.getUrlParam('roadmapId');

let currentSections = [];
let isSectionsExpanded = false;
let reorderState = {
    active: false,
    sourceIndex: null, // Index in the flattened currentSections
    sourceSection: null,
    sourceResourceId: null,
    sourceResourceName: null,
    sourceResourceData: null,
    targetResourceId: null,
    targetResourceName: null,
    longPressTimer: null,
    preventClick: false,
    startPos: { x: 0, y: 0 }
};

function updateReorderHint() {
    let hint = document.getElementById('reorder-hint');
    if (!hint) {
        hint = document.createElement('div');
        hint.id = 'reorder-hint';
        hint.className = 'reorder-hint';
        document.body.appendChild(hint);
    }
    
    hint.innerHTML = `
        <div style="display: flex; flex-direction: column; gap: 4px;">
            <span style="font-weight: 600; font-size: 0.95rem;">Moving "${UI.escapeHtml(reorderState.sourceSection.name)}"</span>
            <span style="font-size: 0.8rem; opacity: 0.9;">Target Resource: <strong>${UI.escapeHtml(reorderState.targetResourceName)}</strong></span>
        </div>
        <div style="display: flex; gap: 8px;">
            <button class="btn btn-secondary btn-sm" onclick="openResourceSelectionModal()" style="padding: 4px 12px; font-size: 0.85rem; background: rgba(255,255,255,0.2); border: none;">Change Resource</button>
            <button class="btn btn-secondary btn-sm" onclick="exitReorderMode()" style="padding: 4px 12px; font-size: 0.85rem; background: rgba(255,255,255,0.2); border: none;">Cancel</button>
        </div>
    `;
}

function enterReorderMode(index) {
    if (reorderState.active) return;
    
    reorderState.active = true;

    // Clear sections search when entering reorder mode
    const searchInput = document.getElementById('sections-search-input');
    if (searchInput) searchInput.value = '';

    reorderState.sourceIndex = index;
    reorderState.sourceSection = currentSections[index];
    reorderState.sourceResourceId = resourceId;
    reorderState.sourceResourceName = resourceName;
    reorderState.sourceResourceData = currentResourceData;
    reorderState.targetResourceId = resourceId;
    reorderState.targetResourceName = resourceName;
    reorderState.preventClick = true;
    
    if (navigator.vibrate) navigator.vibrate(50);

    // Re-render to show gaps
    renderSections(currentSections);

    // Add/update hint
    updateReorderHint();

    // Ensure source section remains in view after gaps are added
    requestAnimationFrame(() => {
        const sourceItem = document.getElementById(`section-${index}`);
        if (sourceItem) {
            sourceItem.scrollIntoView({ behavior: 'smooth', block: 'center' });
        }
    });
}

window.exitReorderMode = async function() {
    const wasResourceChanged = reorderState.active && (reorderState.sourceResourceId !== reorderState.targetResourceId);
    const originalSourceResourceId = reorderState.sourceResourceId;
    const originalSourceResourceName = reorderState.sourceResourceName;
    const originalSourceSectionIndex = reorderState.sourceIndex;

    reorderState.active = false;
    reorderState.sourceIndex = null;
    reorderState.sourceSection = null;
    reorderState.sourceResourceId = null;
    reorderState.sourceResourceName = null;
    reorderState.targetResourceId = null;
    reorderState.targetResourceName = null;
    
    const hint = document.getElementById('reorder-hint');
    if (hint) hint.remove();

    if (wasResourceChanged) {
        // Return to source resource
        resourceId = originalSourceResourceId;
        resourceName = originalSourceResourceName;
        currentResourceData = reorderState.sourceResourceData;

        // Update page header
        const resourceNameTitle = document.getElementById('resource-name');
        if (resourceNameTitle) {
            const icon = UI.getResourceIcon(currentResourceData.type);
            resourceNameTitle.innerHTML = `<span class="resource-icon">${icon}</span> ${UI.escapeHtml(resourceName || 'Resource')}`;
        }
        document.title = `${resourceName || 'Resource'} - Flashback`;
    }

    // Re-render to remove gaps
    await loadSections();

    if (wasResourceChanged) {
        // Scroll to the source section
        requestAnimationFrame(() => {
            const sourceItem = document.getElementById(`section-${originalSourceSectionIndex}`);
            if (sourceItem) {
                sourceItem.scrollIntoView({ behavior: 'smooth', block: 'center' });
            }
        });
    }
};

window.openResourceSelectionModal = function() {
    UI.toggleElement('resource-selection-modal', true);
    const searchInput = document.getElementById('reorder-resource-search-input');
    if (searchInput) {
        searchInput.value = '';
        searchInput.focus();
    }
    const container = document.getElementById('reorder-resource-search-results');
    if (container) container.innerHTML = '';

    // Also clear and load subject resources if subjectId exists
    const subjectContainer = document.getElementById('subject-resources-list');
    const subjectSection = document.getElementById('subject-resources-section');
    if (subjectContainer) subjectContainer.innerHTML = '';
    
    if (subjectId) {
        if (subjectSection) subjectSection.style.display = 'block';
        loadSubjectResources();
    } else {
        if (subjectSection) subjectSection.style.display = 'none';
    }
};

async function loadSubjectResources() {
    try {
        const resources = await client.getResources(subjectId);
        displaySubjectResourceResults(resources);
    } catch (err) {
        console.error('Load subject resources failed:', err);
    }
}

function displaySubjectResourceResults(resources) {
    const container = document.getElementById('subject-resources-list');
    if (!container) return;
    container.innerHTML = '';
    
    if (resources.length === 0) {
        container.innerHTML = '<div class="no-results" style="padding: 1rem; text-align: center; opacity: 0.6;">No resources in this subject.</div>';
        return;
    }

    resources.forEach(res => {
        if (res.id === reorderState.sourceResourceId) {
            return;
        }

        const item = document.createElement('div');
        item.className = 'search-result-item';
        const icon = UI.getResourceIcon(res.type);
        item.innerHTML = `
            <div class="search-result-name"><span class="resource-icon" style="margin-right: 8px;">${icon}</span> ${UI.escapeHtml(res.name)}</div>
        `;
        
        item.onclick = async () => {
            closeResourceSelectionModal();
            await loadTargetResourceSections(res);
        };
        
        container.appendChild(item);
    });
}

function closeResourceSelectionModal() {
    UI.toggleElement('resource-selection-modal', false);
}

async function searchReorderResources(token) {
    try {
        const resources = await client.searchResources(token);
        displayReorderResourceResults(resources);
    } catch (err) {
        console.error('Search resources failed:', err);
    }
}

function displayReorderResourceResults(resources) {
    const container = document.getElementById('reorder-resource-search-results');
    if (!container) return;
    container.innerHTML = '';
    
    resources.forEach(res => {
        if (res.id === reorderState.sourceResourceId) {
            return;
        }

        const item = document.createElement('div');
        item.className = 'search-result-item';
        item.innerHTML = `
            <div class="search-result-name">${UI.escapeHtml(res.name)}</div>
        `;
        
        item.onclick = async () => {
            closeResourceSelectionModal();
            await loadTargetResourceSections(res);
        };
        
        container.appendChild(item);
    });
}

async function loadTargetResourceSections(resource) {
    reorderState.targetResourceId = resource.id;
    reorderState.targetResourceName = resource.name;
    
    // Clear sections search when switching resources in reorder mode
    const searchInput = document.getElementById('sections-search-input');
    if (searchInput) searchInput.value = '';

    // Temporarily change global resourceId and resourceName to load/render sections
    resourceId = resource.id;
    resourceName = resource.name;
    
    // Store full resource data
    currentResourceData = resource;
    
    // Update page header
    const resourceNameHeader = document.getElementById('resource-name');
    if (resourceNameHeader) {
        const icon = UI.getResourceIcon(resource.type);
        resourceNameHeader.innerHTML = `<span class="resource-icon">${icon}</span> ${UI.escapeHtml(resourceName || 'Resource')}`;
    }
    document.title = `${resourceName || 'Resource'} - Flashback`;

    updateReorderHint();
    await loadSections();
}

// Confirmation Modal Functions
(function() {
    const confirmModal = document.getElementById('confirm-modal');
    if (!confirmModal) return;

    const closeConfirmModal = () => {
        confirmModal.style.display = 'none';
        document.body.style.overflow = '';
        UI.setButtonLoading('confirm-modal-btn', false);
    };

    const closeBtn = document.getElementById('close-confirm-modal-btn');
    if (closeBtn) closeBtn.addEventListener('click', closeConfirmModal);

    const cancelBtn = document.getElementById('cancel-confirm-modal-btn');
    if (cancelBtn) cancelBtn.addEventListener('click', closeConfirmModal);

    const confirmBtn = document.getElementById('confirm-modal-btn');
    if (confirmBtn) {
        confirmBtn.addEventListener('click', async () => {
            if (typeof confirmModal._confirmCallback === 'function') {
                UI.setButtonLoading('confirm-modal-btn', true);
                try {
                    await confirmModal._confirmCallback();
                } finally {
                    UI.setButtonLoading('confirm-modal-btn', false);
                    closeConfirmModal();
                }
            } else {
                closeConfirmModal();
            }
        });
    }

    if (confirmModal) {
        confirmModal.addEventListener('click', (e) => {
            if (e.target === confirmModal) closeConfirmModal();
        });
    }

    window.showConfirmModal = (title, message, callback) => {
        const titleEl = document.getElementById('confirm-modal-title');
        const messageEl = document.getElementById('confirm-modal-message');
        if (titleEl) titleEl.textContent = title;
        if (messageEl) messageEl.textContent = message;
        confirmModal._confirmCallback = callback;
        confirmModal.style.display = 'flex';
        document.body.style.overflow = 'hidden';
    };
})();

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    resourceId = UI.getUrlParam('id');
    resourceName = UI.getUrlParam('name');
    if (!resourceId) {
        window.location.href = '/home.html';
        return;
    }

    const typeId = UI.getUrlParam('type');
    const resourceNameHeader = document.getElementById('resource-name');
    if (resourceNameHeader) {
        const icon = UI.getResourceIcon(parseInt(typeId));
        resourceNameHeader.innerHTML = `<span class="resource-icon">${icon}</span> ${UI.escapeHtml(resourceName || 'Resource')}`;
    }
    document.title = `${resourceName || 'Resource'} - Flashback`;

    // Store current resource data from URL params
    currentResourceData = {
        id: resourceId,
        name: resourceName || '',
        type: parseInt(UI.getUrlParam('type') || '0'),
        pattern: parseInt(UI.getUrlParam('pattern') || '0'),
        link: UI.getUrlParam('link') || '',
        production: UI.getUrlParam('production') || '',
        expiration: UI.getUrlParam('expiration') || ''
    };

    // Display breadcrumb
    displayBreadcrumb();

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }


    const addSectionBtn = document.getElementById('add-section-btn');
    if (addSectionBtn) {
        addSectionBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-section-modal', true);
            document.body.style.overflow = 'hidden';
            setTimeout(() => {
                const nameInput = document.getElementById('section-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const closeSectionModalBtn = document.getElementById('close-section-modal-btn');
    if (closeSectionModalBtn) {
        closeSectionModalBtn.addEventListener('click', () => {
            UI.toggleElement('add-section-modal', false);
            document.body.style.overflow = 'auto';
            UI.clearForm('section-form');
        });
    }

    const cancelSectionBtn = document.getElementById('cancel-section-btn');
    if (cancelSectionBtn) {
        cancelSectionBtn.addEventListener('click', () => {
            UI.toggleElement('add-section-modal', false);
            document.body.style.overflow = 'auto';
            UI.clearForm('section-form');
        });
    }

    const addSectionModal = document.getElementById('add-section-modal');
    if (addSectionModal) {
        addSectionModal.addEventListener('click', (e) => {
            if (e.target === addSectionModal) {
                UI.toggleElement('add-section-modal', false);
                document.body.style.overflow = 'auto';
                UI.clearForm('section-form');
            }
        });
    }

    const sectionForm = document.getElementById('section-form');
    if (sectionForm) {
        sectionForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('section-name').value.trim();
            const link = document.getElementById('section-link').value.trim();

            if (!name) {
                UI.showError('Please enter a section name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-section-btn', true);

            try {
                // Position 0 means add to end
                await client.createSection(resourceId, name, link, 0);

                UI.toggleElement('add-section-modal', false);
                document.body.style.overflow = 'auto';
                UI.clearForm('section-form');
                UI.setButtonLoading('save-section-btn', false);

                loadSections();
            } catch (err) {
                console.error('Add section failed:', err);
                UI.showError(err.message || 'Failed to add section');
                UI.setButtonLoading('save-section-btn', false);
            }
        });
    }

    // Pattern options per resource type
    const patternsByType = {
        0: [{value: 0, label: 'Chapter'}, {value: 1, label: 'Page'}], // Book
        1: [{value: 1, label: 'Page'}], // Website
        2: [{value: 2, label: 'Session'}, {value: 3, label: 'Episode'}], // Course
        3: [{value: 3, label: 'Episode'}], // Video
        4: [{value: 4, label: 'Playlist'}, {value: 5, label: 'Post'}], // Channel
        5: [{value: 5, label: 'Post'}], // Mailing List
        6: [{value: 1, label: 'Page'}], // Manual
        7: [{value: 0, label: 'Chapter'}, {value: 1, label: 'Page'}], // Slides
        8: [{value: 6, label: 'Memory'}], // Your Knowledge
    };

    // Edit resource handlers
    const editResourceBtn = document.getElementById('edit-resource-btn');
    const editResourceModal = document.getElementById('edit-resource-modal');
    const closeEditResourceModalBtn = document.getElementById('close-edit-resource-modal-btn');
    const cancelEditResourceBtn = document.getElementById('cancel-edit-resource-btn');

    const closeEditResource = () => {
        UI.toggleElement('edit-resource-modal', false);
        document.body.style.overflow = '';
        UI.clearForm('edit-resource-form');
    };

    if (editResourceBtn) {
        editResourceBtn.addEventListener('click', () => {
            UI.toggleElement('edit-resource-modal', true);
            document.body.style.overflow = 'hidden';

            // Populate form with current data
            document.getElementById('edit-resource-name').value = currentResourceData.name;
            document.getElementById('edit-resource-type').value = currentResourceData.type;
            document.getElementById('edit-resource-link').value = currentResourceData.link;

            // Convert epoch seconds to date string (YYYY-MM-DD)
            const productionDate = currentResourceData.production
                ? new Date(currentResourceData.production * 1000).toISOString().split('T')[0]
                : '';
            const expirationDate = currentResourceData.expiration
                ? new Date(currentResourceData.expiration * 1000).toISOString().split('T')[0]
                : '';

            document.getElementById('edit-resource-production').value = productionDate;
            document.getElementById('edit-resource-expiration').value = expirationDate;

            // Update pattern options based on type and set current pattern
            updatePatternOptions(currentResourceData.type, currentResourceData.pattern);

            setTimeout(() => {
                document.getElementById('edit-resource-name').focus();
            }, 100);
        });
    }
    if (closeEditResourceModalBtn) closeEditResourceModalBtn.addEventListener('click', closeEditResource);
    if (cancelEditResourceBtn) cancelEditResourceBtn.addEventListener('click', closeEditResource);
    if (editResourceModal) {
        editResourceModal.addEventListener('click', (e) => {
            if (e.target === editResourceModal) closeEditResource();
        });
    }

    // Function to update pattern options based on selected type
    function updatePatternOptions(typeValue, selectedPattern = null) {
        const patternSelect = document.getElementById('edit-resource-pattern');
        patternSelect.innerHTML = '';

        if (!typeValue && typeValue !== 0) {
            patternSelect.disabled = true;
            patternSelect.appendChild(new Option('Select type first...', ''));
            return;
        }

        patternSelect.disabled = false;
        const patterns = patternsByType[parseInt(typeValue)] || [];

        if (patterns.length === 1) {
            // If only one pattern, select it automatically
            const option = new Option(patterns[0].label, patterns[0].value, true, true);
            patternSelect.appendChild(option);
        } else {
            patterns.forEach(pattern => {
                const isSelected = selectedPattern !== null && pattern.value === selectedPattern;
                const option = new Option(pattern.label, pattern.value, isSelected, isSelected);
                patternSelect.appendChild(option);
            });
        }
    }

    // Add event listener for type change in edit form
    const editTypeSelect = document.getElementById('edit-resource-type');
    if (editTypeSelect) {
        editTypeSelect.addEventListener('change', () => {
            updatePatternOptions(editTypeSelect.value);
        });
    }


    const editResourceForm = document.getElementById('edit-resource-form');
    if (editResourceForm) {
        editResourceForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('edit-resource-name').value.trim();
            const type = parseInt(document.getElementById('edit-resource-type').value);
            const pattern = parseInt(document.getElementById('edit-resource-pattern').value);
            const link = document.getElementById('edit-resource-link').value.trim();
            const productionDate = document.getElementById('edit-resource-production').value;
            const expirationDate = document.getElementById('edit-resource-expiration').value;

            if (!name || !productionDate || !expirationDate) {
                UI.showError('Please fill all required fields');
                return;
            }

            // Convert dates to epoch seconds
            const production = Math.floor(new Date(productionDate).getTime() / 1000);
            const expiration = Math.floor(new Date(expirationDate).getTime() / 1000);

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-edit-resource-btn', true);

            try {
                await client.editResource(resourceId, type, pattern, name, production, expiration, link);

                closeEditResource();
                UI.setButtonLoading('save-edit-resource-btn', false);

                // Update the page and URL with new data
                currentResourceData = { id: resourceId, name, type, pattern, link, production, expiration };
                document.getElementById('resource-name').textContent = name;
                document.title = `${name} - Flashback`;

                // Update URL params
                const subjectId = UI.getUrlParam('subjectId');
                const subjectName = UI.getUrlParam('subjectName');
                const roadmapId = UI.getUrlParam('roadmapId');
                const roadmapName = UI.getUrlParam('roadmapName');
                const newUrl = `resource.html?id=${resourceId}&name=${encodeURIComponent(name)}&type=${type}&pattern=${pattern}&link=${encodeURIComponent(link)}&production=${production}&expiration=${expiration}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
                window.history.replaceState({}, '', newUrl);

                // Update breadcrumb and re-render sections to update links with the new resource name
                displayBreadcrumb();
                renderSections(currentSections);

                UI.showSuccess('Resource updated successfully');
            } catch (err) {
                console.error('Edit resource failed:', err);
                UI.showError(err.message || 'Failed to edit resource');
                UI.setButtonLoading('save-edit-resource-btn', false);
            }
        });
    }

    // Remove resource handlers
    const removeResourceBtn = document.getElementById('remove-resource-btn');
    const removeResourceModal = document.getElementById('remove-resource-modal');
    const closeRemoveResourceModalBtn = document.getElementById('close-remove-resource-modal-btn');
    const cancelRemoveResourceBtn = document.getElementById('cancel-remove-resource-btn');

    const closeRemoveResource = () => {
        UI.toggleElement('remove-resource-modal', false);
        document.body.style.overflow = '';
    };

    if (removeResourceBtn) {
        removeResourceBtn.addEventListener('click', () => {
            UI.toggleElement('remove-resource-modal', true);
            document.body.style.overflow = 'hidden';
        });
    }
    if (closeRemoveResourceModalBtn) closeRemoveResourceModalBtn.addEventListener('click', closeRemoveResource);
    if (cancelRemoveResourceBtn) cancelRemoveResourceBtn.addEventListener('click', closeRemoveResource);
    if (removeResourceModal) {
        removeResourceModal.addEventListener('click', (e) => {
            if (e.target === removeResourceModal) closeRemoveResource();
        });
    }

    const confirmRemoveResourceBtn = document.getElementById('confirm-remove-resource-btn');
    if (confirmRemoveResourceBtn) {
        confirmRemoveResourceBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-resource-btn', true);

            try {
                await client.removeResource(resourceId);

                closeRemoveResource();
                UI.setButtonLoading('confirm-remove-resource-btn', false);

                // Redirect back to subject or home
                const subjectId = UI.getUrlParam('subjectId');
                if (subjectId) {
                    const subjectName = UI.getUrlParam('subjectName') || '';
                    const roadmapId = UI.getUrlParam('roadmapId') || '';
                    const roadmapName = UI.getUrlParam('roadmapName') || '';
                    const level = UI.getUrlParam('level') || '';
                    const currentTab = UI.getUrlParam('tab') || 'resources';
                    window.location.href = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&level=${level}&tab=${currentTab}`;
                } else {
                    window.location.href = '/home.html';
                }
            } catch (err) {
                console.error('Remove resource failed:', err);
                UI.showError(err.message || 'Failed to remove resource');
                UI.setButtonLoading('confirm-remove-resource-btn', false);
            }
        });
    }

    loadSections();
    
    // Search event listener
    const sectionsSearchInput = document.getElementById('sections-search-input');
    if (sectionsSearchInput) {
        sectionsSearchInput.addEventListener('input', () => {
            renderSections(currentSections);
        });
    }

    // Resource selection modal handlers
    const closeResourceModalBtn = document.getElementById('close-resource-modal-btn');
    if (closeResourceModalBtn) {
        closeResourceModalBtn.addEventListener('click', closeResourceSelectionModal);
    }

    const reorderResourceSearchInput = document.getElementById('reorder-resource-search-input');
    if (reorderResourceSearchInput) {
        let searchTimeout;
        reorderResourceSearchInput.addEventListener('input', (e) => {
            const query = e.target.value.trim();
            clearTimeout(searchTimeout);
            
            const subjectSection = document.getElementById('subject-resources-section');
            if (!query) {
                if (subjectSection) subjectSection.style.display = subjectId ? 'block' : 'none';
                const container = document.getElementById('reorder-resource-search-results');
                if (container) container.innerHTML = '';
                return;
            }

            if (subjectSection) subjectSection.style.display = 'none';
            searchTimeout = setTimeout(() => {
                searchReorderResources(query);
            }, 300);
        });
    }

    // Close on backdrop click
    const resourceSelectionModal = document.getElementById('resource-selection-modal');
    if (resourceSelectionModal) {
        resourceSelectionModal.addEventListener('click', (e) => {
            if (e.target === resourceSelectionModal) closeResourceSelectionModal();
        });
    }
});

async function loadSections() {
    UI.toggleElement('loading', true);
    UI.toggleElement('sections-list', false);
    UI.toggleElement('empty-state', false);
    UI.toggleElement('sections-search-container', false);

    try {
        const sections = await client.getSections(resourceId);

        UI.toggleElement('loading', false);

        if (sections.length === 0 && !reorderState.active) {
            UI.toggleElement('empty-state', true);
            UI.toggleElement('sections-toggle-container', false);
            UI.toggleElement('sections-search-container', false);
        } else {
            UI.toggleElement('empty-state', false);
            UI.toggleElement('sections-list', true);
            UI.toggleElement('sections-search-container', !reorderState.active);
            renderSections(sections);
        }
    } catch (err) {
        console.error('Loading sections failed:', err);
        UI.toggleElement('loading', false);
        UI.showError('Failed to load sections: ' + (err.message || 'Unknown error'));
    }
}

function renderSections(sections) {
    const container = document.getElementById('sections-list');
    const searchInput = document.getElementById('sections-search-input');
    const searchTerm = searchInput ? searchInput.value.toLowerCase().trim() : '';
    container.innerHTML = '';

    if (reorderState.active) {
        container.classList.add('reorder-mode-active');
    } else {
        container.classList.remove('reorder-mode-active');
    }

    // Sort sections by position and store
    currentSections = sections.sort((a, b) => a.position - b.position);

    // Filter by search term
    let filteredSections = currentSections;
    if (searchTerm) {
        filteredSections = currentSections.filter(section => 
            section.name.toLowerCase().includes(searchTerm)
        );
    }
    
    // Handle expansion/collapsing (show only top 3 by default)
    const toggleContainer = document.getElementById('sections-toggle-container');
    const toggleBtn = document.getElementById('sections-toggle-btn');
    
    let displayedSections = filteredSections;
    if (filteredSections.length > 3 && !reorderState.active) {
        UI.toggleElement('sections-toggle-container', true);
        if (!isSectionsExpanded) {
            displayedSections = filteredSections.slice(0, 3);
            toggleBtn.textContent = `Show All (${filteredSections.length})`;
        } else {
            toggleBtn.textContent = 'Show Less';
        }

        // Add event listener if not already added
        if (!toggleBtn.dataset.listenerAdded) {
            toggleBtn.addEventListener('click', () => {
                isSectionsExpanded = !isSectionsExpanded;
                renderSections(currentSections);
            });
            toggleBtn.dataset.listenerAdded = 'true';
        }
    } else {
        UI.toggleElement('sections-toggle-container', false);
    }

    const stateNames = ['draft', 'reviewed', 'completed'];

    const sourceSection = reorderState.active ? reorderState.sourceSection : null;

    const createGap = (pos) => {
        const gap = document.createElement('div');
        gap.className = 'target-block-gap';
        gap.onclick = (e) => {
            e.stopPropagation();
            const targetPos = pos;

            if (reorderState.sourceResourceId === reorderState.targetResourceId && (targetPos === sourceSection.position || targetPos === sourceSection.position + 1)) {
                exitReorderMode();
                return;
            }

            window.showConfirmModal('Confirm Move', `Move "${reorderState.sourceSection.name}" to ${reorderState.targetResourceName}?`, async () => {
                await moveSection(reorderState.sourceResourceId, sourceSection.position, reorderState.targetResourceId, targetPos);
                // After successful move, we should stay in the target resource
                // but exit reorder mode.
                reorderState.sourceResourceId = reorderState.targetResourceId; // Prevent returning to source
                updateUrl();
                exitReorderMode();
            });
        };
        return gap;
    };

    if (reorderState.active && displayedSections.length === 0) {
        container.appendChild(createGap(1));
    }

    displayedSections.forEach((section, index) => {
        if (reorderState.active && (reorderState.sourceResourceId !== reorderState.targetResourceId || (section.position !== sourceSection.position && section.position !== sourceSection.position + 1))) {
            container.appendChild(createGap(section.position));
        }

        const sectionItem = document.createElement('div');
        sectionItem.className = 'item-block compact';
        if (reorderState.active && index === reorderState.sourceIndex) {
            sectionItem.classList.add('reorder-source');
        }
        sectionItem.id = `section-${index}`;
        sectionItem.dataset.position = section.position;


        const startLongPressTimer = (e) => {
            if (reorderState.active) return;
            const touch = e.touches ? e.touches[0] : e;
            reorderState.startPos = { x: touch.clientX, y: touch.clientY };
            reorderState.longPressTimer = setTimeout(() => enterReorderMode(index), 500);
        };

        const clearLongPressTimer = () => {
            if (reorderState.longPressTimer) {
                clearTimeout(reorderState.longPressTimer);
                reorderState.longPressTimer = null;
            }
        };

        sectionItem.addEventListener('mousedown', (e) => {
            if (e.target.closest('button') || e.target.closest('select') || e.target.closest('input') || e.target.closest('a')) return;
            startLongPressTimer(e);
        });
        sectionItem.addEventListener('mouseup', clearLongPressTimer);
        sectionItem.addEventListener('mouseleave', clearLongPressTimer);
        sectionItem.addEventListener('touchstart', (e) => {
            if (e.target.closest('button') || e.target.closest('select') || e.target.closest('input') || e.target.closest('a')) return;
            startLongPressTimer(e);
        }, { passive: true });
        sectionItem.addEventListener('touchend', clearLongPressTimer);
        sectionItem.addEventListener('touchmove', (e) => {
            if (reorderState.longPressTimer) {
                const touch = e.touches[0];
                const dx = touch.clientX - reorderState.startPos.x;
                const dy = touch.clientY - reorderState.startPos.y;
                if (Math.sqrt(dx * dx + dy * dy) > 10) {
                    clearLongPressTimer();
                }
            }
        }, { passive: true });
        sectionItem.addEventListener('touchcancel', clearLongPressTimer);

        const stateName = stateNames[section.state] || 'draft';

        // State badge colors
        const stateColors = {
            'draft': { bg: 'rgba(158, 158, 158, 0.2)', color: '#9e9e9e' },
            'reviewed': { bg: 'rgba(33, 150, 243, 0.2)', color: '#2196f3' },
            'completed': { bg: 'rgba(76, 175, 80, 0.2)', color: '#4caf50' }
        };
        const stateColor = stateColors[stateName] || stateColors['draft'];

        let linkHtml = '';
        if (section.link) {
            linkHtml = `
                <a href="${UI.escapeHtml(section.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()" class="external-link-icon" title="Open Link" style="display: flex; align-items: center; justify-content: center; width: 24px; height: 24px; border-radius: 50%; background: rgba(255,255,255,0.05); transition: background 0.2s;">
                    <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path><polyline points="15 3 21 3 21 9"></polyline><line x1="10" y1="14" x2="21" y2="3"></line></svg>
                </a>
            `;
        }

        sectionItem.innerHTML = `
            <div style="width: 100%; display: flex; flex-direction: column; gap: 0.25rem;">
                <div class="item-header" style="margin-bottom: 0; align-items: center;">
                    <div style="display: flex; align-items: center; gap: var(--space-xs); flex: 1; cursor: pointer;">
                        <span class="item-badge" style="font-size: 10px; height: 18px; min-width: 18px; padding: 0 4px; text-align: center;">${index + 1}</span>
                        <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base);">${UI.escapeHtml(section.name)}</h3>
                    </div>
                    <div style="display: flex; align-items: center; gap: 0.5rem;">
                        <span class="item-badge" style="background: ${stateColor.bg}; color: ${stateColor.color}; text-transform: capitalize; font-size: 10px; height: 18px; min-width: auto; padding: 0 6px;">${UI.escapeHtml(stateName)}</span>
                        ${linkHtml}
                    </div>
                </div>
            </div>
        `;

        // Selection-based reorder click handler
        sectionItem.style.cursor = reorderState.active ? 'default' : 'pointer';
        sectionItem.addEventListener('click', async (e) => {
            if (reorderState.active) {
                if (index === reorderState.sourceIndex) {
                    exitReorderMode();
                }
                return;
            }
            if (reorderState.preventClick) {
                reorderState.preventClick = false;
                return;
            }

            // Don't navigate if clicking on links or other actions
            if (e.target.closest('a') || e.target.closest('button')) {
                return;
            }

            const resourceType = UI.getUrlParam('type');
            const resourcePattern = UI.getUrlParam('pattern');
            const resourceLink = UI.getUrlParam('link');
            const resourceProduction = UI.getUrlParam('production');
            const resourceExpiration = UI.getUrlParam('expiration');
            const subjectId = UI.getUrlParam('subjectId');
            const subjectName = UI.getUrlParam('subjectName');
            const roadmapId = UI.getUrlParam('roadmapId');
            const roadmapName = UI.getUrlParam('roadmapName');
            const milestoneLevel = UI.getUrlParam('level');
            const currentTab = UI.getUrlParam('tab') || 'resources';
            window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${section.position}&sectionState=${section.state}&name=${encodeURIComponent(section.name)}&sectionLink=${encodeURIComponent(section.link || '')}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink || '')}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${milestoneLevel || ''}&tab=${currentTab}`;
        });

        // Add touchcancel to clear long-press
        sectionItem.addEventListener('touchcancel', clearLongPressTimer);

        container.appendChild(sectionItem);

        if (reorderState.active && (reorderState.sourceResourceId !== reorderState.targetResourceId) && index === displayedSections.length - 1) {
            container.appendChild(createGap(section.position + 1));
        } else if (reorderState.active && index === displayedSections.length - 1 && section.position + 1 !== sourceSection.position && section.position + 1 !== sourceSection.position + 1) {
            container.appendChild(createGap(section.position + 1));
        }
    });
}

function updateUrl() {
    const params = new URLSearchParams(window.location.search);
    params.set('id', resourceId);
    params.set('name', resourceName);
    if (currentResourceData) {
        params.set('type', currentResourceData.type);
        params.set('pattern', currentResourceData.pattern);
        params.set('link', currentResourceData.link || '');
        params.set('production', currentResourceData.production || '');
        params.set('expiration', currentResourceData.expiration || '');
    }
    
    const newUrl = `${window.location.pathname}?${params.toString()}`;
    window.history.replaceState({}, '', newUrl);
    displayBreadcrumb();
}

function displayBreadcrumb() {
    const subjectId = UI.getUrlParam('subjectId');
    const subjectName = UI.getUrlParam('subjectName');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');
    const level = UI.getUrlParam('level');
    const resourceType = UI.getUrlParam('type');

    const breadcrumbItems = [];

    if (roadmapId && roadmapName) {
        breadcrumbItems.push({
            name: roadmapName,
            icon: UI.getRoadmapIcon(),
            url: `roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}`
        });
    }

    if (subjectId && subjectName) {
        const currentTab = UI.getUrlParam('tab') || 'resources';
        breadcrumbItems.push({
            name: subjectName,
            url: `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${level}&tab=${currentTab}`
        });
    }

    UI.renderBreadcrumbs(breadcrumbItems);
}

async function removeSection(position) {
    try {
        await client.removeSection(resourceId, position);
        await loadSections();
        UI.showSuccess('Section removed successfully');
    } catch (err) {
        console.error('Remove section failed:', err);
        UI.showError('Failed to remove section: ' + (err.message || 'Unknown error'));
    }
}

async function moveSection(sourceResourceId, sourceSectionPosition, targetResourceId, targetSectionPosition) {
    try {
        await client.moveSection(sourceResourceId, sourceSectionPosition, targetResourceId, targetSectionPosition);
        await loadSections();
        UI.showSuccess('Section moved successfully');
    } catch (err) {
        console.error('Move section failed:', err);
        UI.showError('Failed to move section: ' + (err.message || 'Unknown error'));
    }
}
