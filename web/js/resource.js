// Store current resource data
let currentResourceData = null;

window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const resourceId = UI.getUrlParam('id');
    const resourceName = UI.getUrlParam('name');
    if (!resourceId) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('resource-name').textContent = resourceName || 'Resource';
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
            UI.toggleElement('add-section-form', true);
            setTimeout(() => {
                const nameInput = document.getElementById('section-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const cancelSectionBtn = document.getElementById('cancel-section-btn');
    if (cancelSectionBtn) {
        cancelSectionBtn.addEventListener('click', () => {
            UI.toggleElement('add-section-form', false);
            UI.clearForm('section-form');
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

                UI.toggleElement('add-section-form', false);
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
    if (editResourceBtn) {
        editResourceBtn.addEventListener('click', () => {
            UI.toggleElement('edit-resource-modal', true);

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

    const cancelEditResourceBtn = document.getElementById('cancel-edit-resource-btn');
    if (cancelEditResourceBtn) {
        cancelEditResourceBtn.addEventListener('click', () => {
            UI.toggleElement('edit-resource-modal', false);
            UI.clearForm('edit-resource-form');
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

                UI.toggleElement('edit-resource-modal', false);
                UI.clearForm('edit-resource-form');
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
    if (removeResourceBtn) {
        removeResourceBtn.addEventListener('click', () => {
            UI.toggleElement('remove-resource-modal', true);
        });
    }

    const cancelRemoveResourceBtn = document.getElementById('cancel-remove-resource-btn');
    if (cancelRemoveResourceBtn) {
        cancelRemoveResourceBtn.addEventListener('click', () => {
            UI.toggleElement('remove-resource-modal', false);
        });
    }

    const confirmRemoveResourceBtn = document.getElementById('confirm-remove-resource-btn');
    if (confirmRemoveResourceBtn) {
        confirmRemoveResourceBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-resource-btn', true);

            try {
                await client.removeResource(resourceId);

                UI.toggleElement('remove-resource-modal', false);
                UI.setButtonLoading('confirm-remove-resource-btn', false);

                // Redirect back to subject or home
                const subjectId = UI.getUrlParam('subjectId');
                if (subjectId) {
                    const subjectName = UI.getUrlParam('subjectName') || '';
                    const roadmapId = UI.getUrlParam('roadmapId') || '';
                    const roadmapName = UI.getUrlParam('roadmapName') || '';
                    window.location.href = `subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;
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
});

async function loadSections() {
    UI.toggleElement('loading', true);
    UI.toggleElement('sections-list', false);
    UI.toggleElement('empty-state', false);

    try {
        const resourceId = UI.getUrlParam('id');
        const sections = await client.getSections(resourceId);

        UI.toggleElement('loading', false);

        if (sections.length === 0) {
            UI.toggleElement('empty-state', true);
        } else {
            UI.toggleElement('sections-list', true);
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
    container.innerHTML = '';

    // Sort sections by position
    const sortedSections = sections.sort((a, b) => a.position - b.position);

    const stateNames = ['draft', 'reviewed', 'completed'];

    sortedSections.forEach(section => {
        const sectionItem = document.createElement('div');
        sectionItem.className = 'section-item';
        sectionItem.style.cursor = 'pointer';

        const stateName = stateNames[section.state] || 'draft';

        let html = `
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.5rem;">
                <div class="section-name">${UI.escapeHtml(section.name)}</div>
                <span class="section-state ${stateName}">${UI.escapeHtml(stateName)}</span>
            </div>
        `;

        if (section.link) {
            html += `
                <div class="section-link">
                    <a href="${UI.escapeHtml(section.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()">
                        ${UI.escapeHtml(section.link)}
                    </a>
                </div>
            `;
        }

        sectionItem.innerHTML = html;

        sectionItem.addEventListener('click', () => {
            const resourceId = UI.getUrlParam('id');
            const resourceName = UI.getUrlParam('name') || '';
            const resourceType = UI.getUrlParam('type');
            const resourcePattern = UI.getUrlParam('pattern');
            const resourceLink = UI.getUrlParam('link');
            const resourceProduction = UI.getUrlParam('production');
            const resourceExpiration = UI.getUrlParam('expiration');
            const subjectId = UI.getUrlParam('subjectId');
            const subjectName = UI.getUrlParam('subjectName');
            const roadmapId = UI.getUrlParam('roadmapId');
            const roadmapName = UI.getUrlParam('roadmapName');
            window.location.href = `section-cards.html?resourceId=${resourceId}&sectionPosition=${section.position}&sectionState=${section.state}&name=${encodeURIComponent(section.name)}&resourceName=${encodeURIComponent(resourceName)}&resourceType=${resourceType}&resourcePattern=${resourcePattern}&resourceLink=${encodeURIComponent(resourceLink || '')}&resourceProduction=${resourceProduction}&resourceExpiration=${resourceExpiration}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
        });

        container.appendChild(sectionItem);
    });
}

function displayBreadcrumb() {
    const breadcrumb = document.getElementById('breadcrumb');
    if (!breadcrumb) return;

    const subjectId = UI.getUrlParam('subjectId');
    const subjectName = UI.getUrlParam('subjectName');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');

    let breadcrumbHtml = '';

    if (roadmapId && roadmapName) {
        breadcrumbHtml += `<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`;
    }

    if (subjectId && subjectName) {
        if (breadcrumbHtml) breadcrumbHtml += ' → ';
        breadcrumbHtml += `<a href="subject.html?id=${subjectId}&name=${encodeURIComponent(subjectName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(subjectName)}</a>`;
    }

    if (breadcrumbHtml) {
        breadcrumb.innerHTML = breadcrumbHtml;
    }
}
