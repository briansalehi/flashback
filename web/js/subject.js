// Updated: 2026-02-20 with topic remove/reorder features
window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const subjectId = UI.getUrlParam('id');
    const subjectName = UI.getUrlParam('name');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');

    if (!subjectId) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('subject-name').textContent = subjectName || 'Subject';
    document.title = `${subjectName || 'Subject'} - Flashback`;

    // Ensure action buttons are hidden by default (defensive in case of prior states/styles)
    const initRenameBtn = document.getElementById('rename-subject-btn');
    const initRemoveBtn = document.getElementById('remove-subject-btn');
    if (initRenameBtn) initRenameBtn.style.display = 'none';
    if (initRemoveBtn) initRemoveBtn.style.display = 'none';

    // Reveal edit/remove on title click
    const subjectTitle = document.getElementById('subject-name');
    if (subjectTitle) {
        subjectTitle.setAttribute('tabindex', '0');
        const renameBtn = document.getElementById('rename-subject-btn');
        const removeBtn = document.getElementById('remove-subject-btn');
        const reveal = () => {
            if (renameBtn) renameBtn.style.display = 'inline-block';
            if (removeBtn) removeBtn.style.display = 'inline-block';
        };
        subjectTitle.addEventListener('click', reveal);
        subjectTitle.addEventListener('keydown', (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); reveal(); }});
    }

    // Display breadcrumb
    displayBreadcrumb(roadmapId);

    const signoutBtn = document.getElementById('signout-btn');
    if (signoutBtn) {
        signoutBtn.addEventListener('click', async (e) => {
            e.preventDefault();
            await client.signOut();
            window.location.href = '/index.html';
        });
    }

    // Rename subject handlers
    const renameSubjectBtn = document.getElementById('rename-subject-btn');
    const renameSubjectModal = document.getElementById('rename-subject-modal');
    const cancelRenameSubjectBtn = document.getElementById('cancel-rename-subject-btn');
    const closeRenameSubjectModalBtn = document.getElementById('close-rename-subject-modal-btn');

    if (renameSubjectBtn) {
        renameSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('rename-subject-modal', true);
            document.body.style.overflow = 'hidden';
            document.getElementById('rename-subject-name').value = subjectName || '';
            setTimeout(() => {
                document.getElementById('rename-subject-name').focus();
            }, 100);
        });
    } else {
        console.error('Rename subject button not found');
    }

    const closeRename = () => {
        UI.toggleElement('rename-subject-modal', false);
        document.body.style.overflow = '';
        UI.clearForm('rename-subject-form');
    };
    if (cancelRenameSubjectBtn) {
        cancelRenameSubjectBtn.addEventListener('click', closeRename);
    }
    if (closeRenameSubjectModalBtn) {
        closeRenameSubjectModalBtn.addEventListener('click', closeRename);
    }

    if (renameSubjectModal) {
        renameSubjectModal.addEventListener('click', (e) => {
            if (e.target === renameSubjectModal) {
                closeRename();
            }
        });
    }

    const renameSubjectForm = document.getElementById('rename-subject-form');
    if (renameSubjectForm) {
        renameSubjectForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const newName = document.getElementById('rename-subject-name').value.trim();
            if (!newName) {
                UI.showError('Please enter a subject name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-rename-subject-btn', true);

            try {
                await client.renameSubject(subjectId, newName);

                closeRename();
                UI.setButtonLoading('save-rename-subject-btn', false);

                // Update the URL and page
                const newUrl = `subject.html?id=${subjectId}&name=${encodeURIComponent(newName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(UI.getUrlParam('roadmapName') || '')}&level=${UI.getUrlParam('level') || ''}`;
                window.history.replaceState({}, '', newUrl);
                document.getElementById('subject-name').textContent = newName;
                document.title = `${newName} - Flashback`;

                UI.showSuccess('Subject renamed successfully');
            } catch (err) {
                console.error('Rename subject failed:', err);
                UI.showError(err.message || 'Failed to rename subject');
                UI.setButtonLoading('save-rename-subject-btn', false);
            }
        });
    }

    // Remove subject handlers
    const removeSubjectBtn = document.getElementById('remove-subject-btn');
    const removeSubjectModal = document.getElementById('remove-subject-modal');
    const cancelRemoveSubjectBtn = document.getElementById('cancel-remove-subject-btn');
    const closeRemoveSubjectModalBtn = document.getElementById('close-remove-subject-modal-btn');

    if (removeSubjectBtn) {
        removeSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('remove-subject-modal', true);
            document.body.style.overflow = 'hidden';
        });
    } else {
        console.error('Remove subject button not found');
    }

    const closeRemove = () => {
        UI.toggleElement('remove-subject-modal', false);
        document.body.style.overflow = '';
    };
    if (cancelRemoveSubjectBtn) {
        cancelRemoveSubjectBtn.addEventListener('click', closeRemove);
    }
    if (closeRemoveSubjectModalBtn) {
        closeRemoveSubjectModalBtn.addEventListener('click', closeRemove);
    }

    if (removeSubjectModal) {
        removeSubjectModal.addEventListener('click', (e) => {
            if (e.target === removeSubjectModal) {
                closeRemove();
            }
        });
    }

    const confirmRemoveSubjectBtn = document.getElementById('confirm-remove-subject-btn');
    if (confirmRemoveSubjectBtn) {
        confirmRemoveSubjectBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-subject-btn', true);

            try {
                await client.removeSubject(subjectId);

                closeRemove();
                UI.setButtonLoading('confirm-remove-subject-btn', false);

                // Redirect back to roadmap or home
                if (roadmapId) {
                    const roadmapName = UI.getUrlParam('roadmapName') || '';
                    window.location.href = `roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}`;
                } else {
                    window.location.href = '/home.html';
                }
            } catch (err) {
                console.error('Remove subject failed:', err);
                UI.showError(err.message || 'Failed to remove subject');
                UI.setButtonLoading('confirm-remove-subject-btn', false);
            }
        });
    }

    const dropResourceModal = document.getElementById('drop-resource-modal');
    const cancelDropResourceBtn = document.getElementById('cancel-drop-resource-btn');
    const closeDropResourceModalBtn = document.getElementById('close-drop-resource-modal-btn');
    const confirmDropResourceBtn = document.getElementById('confirm-drop-resource-btn');
    let resourceToDropId = null;

    const closeDropResourceModal = () => {
        UI.toggleElement('drop-resource-modal', false);
        document.body.style.overflow = '';
        resourceToDropId = null;
    };

    if (cancelDropResourceBtn) cancelDropResourceBtn.addEventListener('click', closeDropResourceModal);
    if (closeDropResourceModalBtn) closeDropResourceModalBtn.addEventListener('click', (e) => {
        e.preventDefault();
        closeDropResourceModal();
    });
    if (dropResourceModal) {
        dropResourceModal.addEventListener('click', (e) => {
            if (e.target === dropResourceModal) closeDropResourceModal();
        });
    }

    if (confirmDropResourceBtn) {
        confirmDropResourceBtn.addEventListener('click', async () => {
            if (!resourceToDropId) return;

            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-drop-resource-btn', true);

            try {
                await client.dropResourceFromSubject(resourceToDropId, parseInt(subjectId));
                closeDropResourceModal();
                UI.setButtonLoading('confirm-drop-resource-btn', false);
                loadResources();
                UI.showSuccess('Resource dropped successfully');
            } catch (err) {
                console.error('Drop resource failed:', err);
                UI.showError('Failed to drop resource: ' + (err.message || 'Unknown error'));
                UI.setButtonLoading('confirm-drop-resource-btn', false);
            }
        });
    }

    window.openDropResourceModal = (id, name) => {
        resourceToDropId = id;
        const nameEl = document.getElementById('resource-to-drop-name');
        if (nameEl) nameEl.textContent = name;
        UI.toggleElement('drop-resource-modal', true);
        document.body.style.overflow = 'hidden';
    };

    // Practice mode functionality
    const startPracticeBtn = document.getElementById('start-practice-btn');
    if (startPracticeBtn) {
        if (!roadmapId) {
            startPracticeBtn.disabled = true;
            startPracticeBtn.title = 'Access this subject from a roadmap to enable practice mode';
            startPracticeBtn.style.opacity = '0.5';
            startPracticeBtn.style.cursor = 'not-allowed';
        } else {
            startPracticeBtn.addEventListener('click', async (e) => {
                e.preventDefault();
                await startPracticeMode();
            });
        }
    }

    const startAssessmentBtn = document.getElementById('start-assessment-btn');
    if (startAssessmentBtn) {
        if (!roadmapId) {
            startAssessmentBtn.disabled = true;
            startAssessmentBtn.title = 'Access this subject from a roadmap to enable assessment practice';
            startAssessmentBtn.style.opacity = '0.5';
            startAssessmentBtn.style.cursor = 'not-allowed';
        } else {
            startAssessmentBtn.addEventListener('click', async (e) => {
                e.preventDefault();
                await startAssessmentPractice();
            });
        }
    }

    // Tab switching
    const tabs = document.querySelectorAll('.tab');
    const tabContents = document.querySelectorAll('.tab-content');

    function activateTab(targetTab, updateUrl = true) {
        // Update active tab
        tabs.forEach(tab => {
            if (tab.dataset.tab === targetTab) {
                tab.classList.add('active');
            } else {
                tab.classList.remove('active');
            }
        });

        // Update active content
        tabContents.forEach(content => {
            if (content.id === `${targetTab}-content`) {
                content.classList.add('active');
            } else {
                content.classList.remove('active');
            }
        });

        // Toggle "Add Topic" button visibility (only show on Topics tab)
        const addTopicBtn = document.getElementById('add-topic-btn');
        if (addTopicBtn) {
            addTopicBtn.style.display = (targetTab === 'topics') ? 'block' : 'none';
        }

        // Update URL if requested
        const currentUrl = new URL(window.location.href);
        if (updateUrl) {
            currentUrl.searchParams.set('tab', targetTab);
            window.history.replaceState({}, '', currentUrl.toString());
        }

        // Update navigation links to include current tab
        updateChildNavigationLinks(targetTab);

        // Load data when switching tabs
        if (targetTab === 'topics' && !topicsLoaded) {
            loadTopics();
        } else if (targetTab === 'assessments' && !assessmentsLoaded) {
            loadAssessments();
        } else if (targetTab === 'resources' && !resourcesLoaded) {
            loadResources();
        }
    }

    function updateChildNavigationLinks(currentTab) {
        // This function will be called whenever the tab changes to update any static links 
        // that might be on the page, although most are dynamic.
        // For now, we'll rely on dynamic generation in render functions.
    }

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            activateTab(tab.dataset.tab);
        });
    });

    // Topics functionality
    const addTopicBtn = document.getElementById('add-topic-btn');
    const topicOverlay = document.getElementById('add-topic-form-overlay');
    const topicModal = document.getElementById('add-topic-form-modal');

    if (addTopicBtn) {
        addTopicBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-topic-form-overlay', true);
            document.body.style.overflow = 'hidden'; // Prevent scrolling
            setTimeout(() => {
                const nameInput = document.getElementById('topic-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const closeTopicModal = () => {
        UI.toggleElement('add-topic-form-overlay', false);
        UI.clearForm('topic-form');
        document.body.style.overflow = ''; // Restore scrolling
    };

    const cancelTopicBtn = document.getElementById('cancel-topic-btn');
    const closeTopicModalBtn = document.getElementById('close-topic-modal-btn');

    if (cancelTopicBtn) {
        cancelTopicBtn.addEventListener('click', closeTopicModal);
    }
    if (closeTopicModalBtn) {
        closeTopicModalBtn.addEventListener('click', closeTopicModal);
    }

    if (topicOverlay) {
        topicOverlay.addEventListener('click', (e) => {
            if (e.target === topicOverlay) {
                closeTopicModal();
            }
        });
    }



    const topicForm = document.getElementById('topic-form');
    if (topicForm) {
        topicForm.addEventListener('submit', async (e) => {
            e.preventDefault();

            const name = document.getElementById('topic-name').value.trim();
            const levelRadio = document.querySelector('input[name="topic-level"]:checked');
            const level = levelRadio ? parseInt(levelRadio.value) : 0;

            if (!name) {
                UI.showError('Please enter a topic name');
                return;
            }

            UI.hideMessage('error-message');
            UI.setButtonLoading('save-topic-btn', true);

            try {
                // Position 0 means add to end
                await client.createTopic(subjectId, name, level, 0);

                closeTopicModal();
                UI.setButtonLoading('save-topic-btn', false);

                loadTopics();
            } catch (err) {
                console.error('Add topic failed:', err);
                UI.showError(err.message || 'Failed to add topic');
                UI.setButtonLoading('save-topic-btn', false);
            }
        });
    }

    // Pattern options per resource type
    const patternsByType = {
        0: [{value: 0, label: 'Chapters'}, {value: 1, label: 'Pages'}, {value: 7, label: 'Sections'}], // Book
        1: [{value: 1, label: 'Pages'}], // Website
        2: [{value: 2, label: 'Sessions'}, {value: 3, label: 'Episodes'}], // Course
        3: [{value: 3, label: 'Episodes'}], // Video
        4: [{value: 4, label: 'Playlist'}, {value: 5, label: 'Posts'}], // Channel
        5: [{value: 5, label: 'Posts'}], // Mailing List
        6: [{value: 1, label: 'Pages'}], // Manual
        7: [{value: 0, label: 'Chapters'}, {value: 1, label: 'Pages'}], // Slides
        8: [{value: 6, label: 'Memories'}], // Nerves
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
    const resourceOverlay = document.getElementById('add-resource-form-overlay');

    if (addResourceBtn) {
        addResourceBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-resource-form-overlay', true);
            document.body.style.overflow = 'hidden'; // Prevent scrolling
            // Always start in search mode
            UI.toggleElement('search-resource-section', true);
            UI.toggleElement('create-resource-section', false);
            setTimeout(() => {
                const searchInput = document.getElementById('search-resource-input');
                if (searchInput) {
                    searchInput.focus();
                }
            }, 100);
        });
    }

    const closeResourceModal = () => {
        UI.toggleElement('add-resource-form-overlay', false);
        UI.clearForm('resource-form');
        document.getElementById('search-resource-input').value = '';
        document.getElementById('search-resource-results').innerHTML = '';
        UI.toggleElement('no-resource-results', false);
        UI.toggleElement('confirm-add-resource-btn', false);
        window.currentlySelectedResource = null;
        resetPatternSelect();
        document.body.style.overflow = ''; // Restore scrolling
    };

    const cancelResourceModalBtn = document.getElementById('cancel-resource-modal-btn');
    if (cancelResourceModalBtn) {
        cancelResourceModalBtn.addEventListener('click', closeResourceModal);
    }

    const closeResourceModalBtn = document.getElementById('close-resource-modal-btn');
    if (closeResourceModalBtn) {
        closeResourceModalBtn.addEventListener('click', closeResourceModal);
    }

    // Click outside to close resource modal
    if (resourceOverlay) {
        resourceOverlay.addEventListener('click', (e) => {
            if (e.target === resourceOverlay) {
                closeResourceModal();
            }
        });
    }

    const showCreateResourceBtn = document.getElementById('show-create-resource-btn');
    if (showCreateResourceBtn) {
        showCreateResourceBtn.addEventListener('click', () => {
            UI.toggleElement('search-resource-section', false);
            UI.toggleElement('create-resource-section', true);
            setTimeout(() => {
                const nameInput = document.getElementById('resource-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const backToResourceSearchBtn = document.getElementById('back-to-resource-search-btn');
    if (backToResourceSearchBtn) {
        backToResourceSearchBtn.addEventListener('click', () => {
            UI.toggleElement('search-resource-section', true);
            UI.toggleElement('create-resource-section', false);
            UI.toggleElement('confirm-add-resource-btn', false);
        });
    }

    // Confirmation button for resource search
    const confirmAddResourceBtn = document.getElementById('confirm-add-resource-btn');
    window.currentlySelectedResource = null;

    if (confirmAddResourceBtn) {
        confirmAddResourceBtn.addEventListener('click', async () => {
            if (!window.currentlySelectedResource) return;

            UI.setButtonLoading('confirm-add-resource-btn', true);
            try {
                await client.addResourceToSubject(parseInt(subjectId), window.currentlySelectedResource.id);

                closeResourceModal();
                loadResources(); // Reload the resources list
                UI.showSuccess(`Resource "${window.currentlySelectedResource.name}" added successfully`);
            } catch (err) {
                console.error('Add resource failed:', err);
                UI.showError('Failed to add resource: ' + (err.message || 'Unknown error'));
            } finally {
                UI.setButtonLoading('confirm-add-resource-btn', false);
            }
        });
    }

    // Search resources functionality
    const searchResourceInput = document.getElementById('search-resource-input');
    if (searchResourceInput) {
        let searchTimeout;
        searchResourceInput.addEventListener('input', (e) => {
            clearTimeout(searchTimeout);
            const query = e.target.value.trim();

            if (query.length < 2) {
                document.getElementById('search-resource-results').innerHTML = '';
                UI.toggleElement('no-resource-results', false);
                UI.toggleElement('confirm-add-resource-btn', false);
                return;
            }

            searchTimeout = setTimeout(async () => {
                window.currentlySelectedResource = null;
                UI.toggleElement('confirm-add-resource-btn', false);
                // Ensure results container is cleared when search starts
                const resultsContainer = document.getElementById('search-resource-results');
                if (resultsContainer) resultsContainer.innerHTML = '';
                await searchAndDisplayResources(query);
            }, 300);
        });
    }

    function resetPatternSelect() {
        patternSelect.disabled = true;
        patternSelect.innerHTML = '<option value="">Select type first...</option>';
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

                closeResourceModal();
                UI.setButtonLoading('save-resource-btn', false);

                loadResources();
            } catch (err) {
                console.error('Add resource failed:', err);
                UI.showError(err.message || 'Failed to add resource');
                UI.setButtonLoading('save-resource-btn', false);
            }
        });
    }

    // Load active tab (from URL or default to topics)
    let topicsLoaded = false;
    let assessmentsLoaded = false;
    let resourcesLoaded = false;

    const initialTab = UI.getUrlParam('tab') || 'topics';
    activateTab(initialTab, false);
});

async function loadTopics() {
    UI.toggleElement('topics-loading', true);
    UI.toggleElement('topics-list', false);
    UI.toggleElement('topics-empty-state', false);

    try {
        const milestoneLevel = parseInt(UI.getUrlParam('level')) || 0;
        const subjectId = UI.getUrlParam('id');

        // Fetch topics for all levels from 0 up to and including the milestone level
        const allTopics = [];
        for (let level = 0; level <= milestoneLevel; level++) {
            const topics = await client.getTopics(subjectId, level);
            allTopics.push(...topics);
        }

        UI.toggleElement('topics-loading', false);
        topicsLoaded = true;

        if (allTopics.length === 0) {
            UI.toggleElement('topics-empty-state', true);
        } else {
            UI.toggleElement('topics-list', true);
            renderTopics(allTopics, milestoneLevel);
        }
    } catch (err) {
        console.error('Loading topics failed:', err);
        UI.toggleElement('topics-loading', false);
        UI.showError('Failed to load topics: ' + (err.message || 'Unknown error'));
    }
}

function renderTopics(topics, maxLevel) {
    const container = document.getElementById('topics-list');
    container.innerHTML = '';

    const levelInfo = [
        { name: 'Surface', description: 'What you need for a complete understanding' },
        { name: 'Depth', description: 'This is where you dig into very details' },
        { name: 'Origin', description: 'Here you will have enough to be a creator' }
    ];

    // Group topics by level
    const topicsByLevel = { 0: [], 1: [], 2: [] };
    topics.forEach(topic => {
        if (topicsByLevel[topic.level] !== undefined) {
            topicsByLevel[topic.level].push(topic);
        }
    });

    // Render levels from 0 (Surface) up to maxLevel, displaying from bottom to top visually
    const levelsToShow = [];
    for (let i = 0; i <= maxLevel; i++) {
        levelsToShow.push(i);
    }

    levelsToShow.forEach(level => {
        if (topicsByLevel[level] && topicsByLevel[level].length > 0) {
            const levelSection = document.createElement('div');
            levelSection.className = 'level-section';

            const levelHeader = document.createElement('div');
            levelHeader.className = 'level-header';
            levelHeader.innerHTML = `
                ${UI.escapeHtml(levelInfo[level].name)} — ${UI.escapeHtml(levelInfo[level].description)}
            `;
            levelSection.appendChild(levelHeader);

            // Sort topics by position within each level
            const sortedTopics = topicsByLevel[level].sort((a, b) => a.position - b.position);

            sortedTopics.forEach((topic, index) => {
                const topicItem = document.createElement('div');
                topicItem.className = 'item-block compact';
                topicItem.draggable = true;
                topicItem.dataset.position = topic.position;
                topicItem.dataset.level = topic.level;
                topicItem.innerHTML = `
                    <div class="item-header" style="margin-bottom: 0; align-items: flex-start; flex-wrap: wrap; gap: var(--space-xs);">
                        <div style="display: flex; align-items: flex-start; gap: var(--space-xs); flex: 1; min-width: 150px; pointer-events: none;">
                            <span class="item-badge" style="font-size: 10px; height: 18px; min-width: 18px; padding: 0 4px; text-align: center;">${index + 1}</span>
                            <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); overflow-wrap: break-word; word-break: break-word;">${UI.escapeHtml(topic.name)}</h3>
                        </div>
                        <span class="item-badge" style="background: rgba(102, 126, 234, 0.2); color: var(--color-primary-start); font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; margin-left: auto; pointer-events: none;">${UI.escapeHtml(levelInfo[level].name)}</span>
                    </div>
                `;

                const subjectId = UI.getUrlParam('id');
                const subjectName = UI.getUrlParam('name');
                const roadmapId = UI.getUrlParam('roadmapId');
                const roadmapName = UI.getUrlParam('roadmapName');

                // Click to navigate (but not when dragging)
                let isDragging = false;
                topicItem.style.cursor = 'pointer';
                topicItem.addEventListener('click', (e) => {
                    if (!isDragging) {
                        const milestoneLevel = UI.getUrlParam('level') || '0';
                        const currentTab = UI.getUrlParam('tab') || 'topics';
                        window.location.href = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topic.position}&topicLevel=${topic.level}&name=${encodeURIComponent(topic.name)}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&milestoneLevel=${milestoneLevel}&tab=${currentTab}`;
                    }
                });

                // Drag and drop handlers
                topicItem.addEventListener('dragstart', (e) => {
                    isDragging = true;
                    topicItem.classList.add('dragging');
                    e.dataTransfer.effectAllowed = 'move';
                    e.dataTransfer.setData('text/plain', JSON.stringify({
                        position: topic.position,
                        level: topic.level
                    }));
                });

                topicItem.addEventListener('dragend', () => {
                    topicItem.classList.remove('dragging');
                    setTimeout(() => {
                        isDragging = false;
                    }, 100);
                });

                topicItem.addEventListener('dragover', (e) => {
                    e.preventDefault();
                    e.dataTransfer.dropEffect = 'move';

                    const draggingCard = document.querySelector('.dragging');
                    if (draggingCard && draggingCard !== topicItem) {
                        const draggingData = JSON.parse(e.dataTransfer.getData('text/plain') || '{}');
                        // Only allow drag-drop within same level
                        if (draggingData.level === topic.level) {
                            topicItem.classList.add('drag-over');
                        }
                    }
                });

                topicItem.addEventListener('dragleave', () => {
                    topicItem.classList.remove('drag-over');
                });

                topicItem.addEventListener('drop', async (e) => {
                    e.preventDefault();
                    topicItem.classList.remove('drag-over');

                    const dragData = JSON.parse(e.dataTransfer.getData('text/plain') || '{}');
                    const sourcePosition = dragData.position;
                    const sourceLevel = dragData.level;
                    const targetPosition = topic.position;
                    const targetLevel = topic.level;

                    // Only reorder within same level
                    if (sourceLevel === targetLevel && sourcePosition !== targetPosition) {
                        await reorderTopic(sourceLevel, sourcePosition, targetPosition);
                    }
                });

                // Touch event handlers for mobile drag-and-drop
                let touchStartY = 0;
                let touchStartX = 0;
                let touchCurrentY = 0;
                let touchStartElement = null;
                let touchClone = null;
                let touchTargetElement = null;
                let longPressTimer = null;
                let isTouchDragEnabled = false;

                topicItem.addEventListener('touchstart', (e) => {
                    touchStartY = e.touches[0].clientY;
                    touchStartX = e.touches[0].clientX;
                    touchCurrentY = touchStartY;
                    touchStartElement = topicItem;
                    isTouchDragEnabled = false;

                    // Start long-press timer (500ms)
                    longPressTimer = setTimeout(() => {
                        isTouchDragEnabled = true;

                        // Vibrate for feedback if available
                        if (navigator.vibrate) {
                            navigator.vibrate(50);
                        }

                        // Create a visual clone for dragging
                        touchClone = topicItem.cloneNode(true);
                        touchClone.style.position = 'fixed';
                        touchClone.style.top = topicItem.getBoundingClientRect().top + 'px';
                        touchClone.style.left = topicItem.getBoundingClientRect().left + 'px';
                        touchClone.style.width = topicItem.offsetWidth + 'px';
                        touchClone.style.opacity = '0.8';
                        touchClone.style.pointerEvents = 'none';
                        touchClone.style.zIndex = '1000';
                        touchClone.classList.add('dragging');
                        document.body.appendChild(touchClone);

                        topicItem.style.opacity = '0.3';
                    }, 500);
                });

                topicItem.addEventListener('touchmove', (e) => {
                    if (!touchStartElement) return;

                    // Calculate movement distance
                    const deltaX = Math.abs(e.touches[0].clientX - touchStartX);
                    const deltaY = Math.abs(e.touches[0].clientY - touchStartY);

                    // If user moved significantly before long-press completed, cancel it (they're scrolling)
                    if (!isTouchDragEnabled && (deltaX > 10 || deltaY > 10)) {
                        if (longPressTimer) {
                            clearTimeout(longPressTimer);
                            longPressTimer = null;
                        }
                        touchStartElement = null;
                        return;
                    }

                    // Only proceed if drag is enabled
                    if (!isTouchDragEnabled || !touchClone) return;

                    e.preventDefault(); // Prevent scrolling while dragging
                    touchCurrentY = e.touches[0].clientY;
                    const dragDeltaY = touchCurrentY - touchStartY;

                    // Move the clone
                    const rect = touchStartElement.getBoundingClientRect();
                    touchClone.style.top = (rect.top + dragDeltaY) + 'px';

                    // Find the element under the touch point
                    touchClone.style.display = 'none';
                    const elementBelow = document.elementFromPoint(e.touches[0].clientX, e.touches[0].clientY);
                    touchClone.style.display = 'block';

                    const itemBelow = elementBelow ? elementBelow.closest('.item-block') : null;

                    // Remove drag-over class from all items
                    document.querySelectorAll('.item-block').forEach(i => i.classList.remove('drag-over'));

                    // Only allow drag-drop within same level
                    if (itemBelow && itemBelow !== touchStartElement) {
                        const itemBelowLevel = parseInt(itemBelow.dataset.level);
                        const touchStartLevel = parseInt(touchStartElement.dataset.level);
                        if (itemBelowLevel === touchStartLevel) {
                            touchTargetElement = itemBelow;
                            itemBelow.classList.add('drag-over');
                        } else {
                            touchTargetElement = null;
                        }
                    } else {
                        touchTargetElement = null;
                    }
                });

                topicItem.addEventListener('touchend', async (e) => {
                    // Clear the long-press timer if it hasn't fired yet
                    if (longPressTimer) {
                        clearTimeout(longPressTimer);
                        longPressTimer = null;
                    }

                    // Only proceed if drag was enabled
                    if (!isTouchDragEnabled) {
                        touchStartElement = null;
                        return;
                    }

                    if (!touchStartElement || !touchClone) return;

                    e.preventDefault();

                    // Remove the clone
                    if (touchClone && touchClone.parentNode) {
                        touchClone.parentNode.removeChild(touchClone);
                    }

                    // Restore opacity
                    touchStartElement.style.opacity = '1';

                    // Remove drag-over class from all items
                    document.querySelectorAll('.item-block').forEach(i => i.classList.remove('drag-over'));

                    // Perform the reorder if dropped on a different item in same level
                    if (touchTargetElement && touchTargetElement !== touchStartElement) {
                        const sourcePosition = parseInt(touchStartElement.dataset.position);
                        const sourceLevel = parseInt(touchStartElement.dataset.level);
                        const targetPosition = parseInt(touchTargetElement.dataset.position);
                        const targetLevel = parseInt(touchTargetElement.dataset.level);

                        if (!isNaN(sourcePosition) && !isNaN(targetPosition) && !isNaN(sourceLevel) && !isNaN(targetLevel)) {
                            if (sourceLevel === targetLevel && sourcePosition !== targetPosition) {
                                await reorderTopic(sourceLevel, sourcePosition, targetPosition);
                            }
                        }
                    }

                    // Reset touch state
                    touchStartY = 0;
                    touchStartX = 0;
                    touchCurrentY = 0;
                    touchStartElement = null;
                    touchClone = null;
                    touchTargetElement = null;
                    isTouchDragEnabled = false;
                });

                topicItem.addEventListener('touchcancel', () => {
                    // Clear the long-press timer
                    if (longPressTimer) {
                        clearTimeout(longPressTimer);
                        longPressTimer = null;
                    }

                    if (touchClone && touchClone.parentNode) {
                        touchClone.parentNode.removeChild(touchClone);
                    }
                    if (touchStartElement) {
                        touchStartElement.style.opacity = '1';
                    }
                    document.querySelectorAll('.item-block').forEach(i => i.classList.remove('drag-over'));
                    touchStartY = 0;
                    touchStartX = 0;
                    touchCurrentY = 0;
                    touchStartElement = null;
                    touchClone = null;
                    touchTargetElement = null;
                    isTouchDragEnabled = false;
                });

                levelSection.appendChild(topicItem);
            });

            container.appendChild(levelSection);
        }
    });
}

async function loadResources() {
    UI.toggleElement('resources-loading', true);
    UI.toggleElement('resources-list', false);
    UI.toggleElement('resources-empty-state', false);

    try {
        const subjectId = parseInt(UI.getUrlParam('id'));
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
        resourceItem.className = 'item-block compact';
        resourceItem.style.cursor = 'pointer';

        // Convert epoch seconds to readable dates
        const productionDate = resource.production ? new Date(resource.production * 1000).toLocaleDateString() : 'N/A';
        const expirationDate = resource.expiration ? new Date(resource.expiration * 1000).toLocaleDateString() : 'N/A';

        const typeName = typeNames[resource.type] || 'Unknown';
        const patternName = patternNames[resource.pattern] || 'Unknown';

        resourceItem.innerHTML = `
            <div style="width: 100%; display: flex; flex-direction: column; gap: 0.25rem; pointer-events: none;">
                <div class="item-header" style="margin-bottom: 0; align-items: flex-start; flex-wrap: wrap; gap: var(--space-xs);">
                    <div style="display: flex; align-items: flex-start; gap: var(--space-xs); flex: 1; min-width: 180px; pointer-events: none;">
                        <h3 class="item-title" style="margin: 0; font-size: var(--font-size-base); overflow-wrap: break-word; word-break: break-word;">${UI.escapeHtml(resource.name)}</h3>
                    </div>
                    <div style="display: flex; gap: var(--space-xs); align-items: center; flex-shrink: 0; margin-left: auto; pointer-events: auto;">
                        <span class="item-badge" style="font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; pointer-events: none;">${UI.escapeHtml(typeName)}</span>
                        <span class="item-badge" style="background: rgba(102, 126, 234, 0.2); color: var(--color-primary-start); font-size: 10px; height: 18px; min-width: auto; padding: 0 6px; pointer-events: none;">${UI.escapeHtml(patternName)}</span>
                        <a href="${UI.escapeHtml(resource.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()" class="external-link-icon" title="Open Link" style="display: flex; align-items: center; justify-content: center; width: 28px; height: 28px; border-radius: 50%; background: rgba(255,255,255,0.05); transition: background 0.2s;">
                            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"></path><polyline points="15 3 21 3 21 9"></polyline><line x1="10" y1="14" x2="21" y2="3"></line></svg>
                        </a>
                        <button class="btn btn-secondary drop-resource-btn" data-resource-id="${resource.id}" style="background-color: #dc3545; color: white; padding: 0.2rem 0.6rem; font-size: 10px; height: 22px; min-width: auto; white-space: nowrap; border: none;">Drop</button>
                    </div>
                </div>
                <div class="item-footer" style="margin-top: 0; padding-top: 0.25rem; border-top: none; justify-content: flex-start; gap: var(--space-md); opacity: 0.8; align-items: center; pointer-events: none;">
                    <div class="item-meta" style="font-size: 11px;">
                        <span>📅 ${UI.escapeHtml(productionDate)}</span>
                    </div>
                    <div class="item-meta" style="font-size: 11px;">
                        <span>⏰ ${UI.escapeHtml(expirationDate)}</span>
                    </div>
                </div>
            </div>
        `;

        // Make the entire resource item clickable to go to resource page
        resourceItem.addEventListener('click', () => {
            const currentTab = UI.getUrlParam('tab') || 'topics';
            const level = UI.getUrlParam('level') || '0';
            const subjectId = UI.getUrlParam('id');
            const subjectName = UI.getUrlParam('name');
            const roadmapId = UI.getUrlParam('roadmapId');
            const roadmapName = UI.getUrlParam('roadmapName');
            window.location.href = `resource.html?id=${resource.id}&name=${encodeURIComponent(resource.name)}&type=${resource.type}&pattern=${resource.pattern}&link=${encodeURIComponent(resource.link)}&production=${resource.production}&expiration=${resource.expiration}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}&level=${level}&tab=${currentTab}`;
        });

        // Add drop button handler
        const dropBtn = resourceItem.querySelector('.drop-resource-btn');
        dropBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            window.openDropResourceModal(resource.id, resource.name);
        });

        container.appendChild(resourceItem);
    });
}

async function startPracticeMode() {
    const level = UI.getUrlParam('level');
    const subjectId = UI.getUrlParam('id');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');

    if (!roadmapId || !subjectId) {
        UI.showError('Missing roadmap or subject information');
        return;
    }

    UI.setButtonLoading('start-practice-btn', true);

    try {
        const topics = await client.getPracticeTopics(parseInt(roadmapId), parseInt(subjectId), level);

        if (topics.length === 0) {
            UI.showError('No topics available for practice');
            UI.setButtonLoading('start-practice-btn', false);
            return;
        }

        // Store practice state in sessionStorage
        sessionStorage.setItem('practiceState', JSON.stringify({
            roadmapId: parseInt(roadmapId),
            subjectId: parseInt(subjectId),
            milestoneLevel: level,
            topics: topics,
            currentTopicIndex: 0,
            practiceMode: 'aggressive'
        }));

        // Find first topic with cards
        let firstTopicWithCards = null;
        let cards = [];

        for (let i = 0; i < topics.length; i++) {
            const topic = topics[i];
            const topicCards = await client.getPracticeCards(
                parseInt(roadmapId),
                parseInt(subjectId),
                topic.level,
                topic.position
            );

            if (topicCards.length > 0) {
                firstTopicWithCards = topic;
                cards = topicCards;
                break;
            }
        }

        if (!firstTopicWithCards || cards.length === 0) {
            UI.showError('No cards available for practice');
            UI.setButtonLoading('start-practice-btn', false);
            return;
        }

        // Update practice state with cards
        const practiceState = JSON.parse(sessionStorage.getItem('practiceState'));
        practiceState.currentCards = cards;
        sessionStorage.setItem('practiceState', JSON.stringify(practiceState));

        // Navigate to first card with context
        // Get milestone info (when coming from roadmap, subjectId is actually milestoneId)
        const milestoneId = UI.getUrlParam('id'); // This is the milestone ID (same as subjectId)
        const milestoneLevel = UI.getUrlParam('level'); // This is the milestone level
        const subjectNameParam = UI.getUrlParam('name') || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';

        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=aggressive&cardIndex=0&totalCards=${cards.length}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectNameParam)}&topicName=${encodeURIComponent(firstTopicWithCards.name)}&milestoneId=${milestoneId}&milestoneLevel=${milestoneLevel}`;
    } catch (err) {
        console.error('Start practice failed:', err);
        UI.showError('Failed to start practice: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('start-practice-btn', false);
    }
}

async function displayBreadcrumb(roadmapId) {
    const breadcrumb = document.getElementById('breadcrumb');
    if (!breadcrumb || !roadmapId) return;

    try {
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        if (roadmapName) {
            breadcrumb.innerHTML = `<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`;
        }
    } catch (err) {
        console.error('Failed to display breadcrumb:', err);
    }
}

async function searchAndDisplayResources(query) {
    const resultsContainer = document.getElementById('search-resource-results');
    const loadingIndicator = document.getElementById('search-resource-results-loading');
    const noResults = document.getElementById('no-resource-results');
    const confirmationBtn = document.getElementById('confirm-add-resource-btn');

    if (loadingIndicator) loadingIndicator.style.display = 'block';
    if (noResults) noResults.style.display = 'none';
    if (confirmationBtn) confirmationBtn.style.display = 'none';
    
    // Do not clear resultsContainer here, it's already cleared in the timeout before calling this
    // to prevent flickering of old results while loading

    try {
        const results = await client.searchResources(query);

        if (loadingIndicator) loadingIndicator.style.display = 'none';

        if (!results || results.length === 0) {
            if (noResults) noResults.style.display = 'block';
            return;
        }

        const typeNames = ['Book', 'Website', 'Course', 'Video', 'Channel', 'Mailing List', 'Manual', 'Slides', 'Your Knowledge'];
        const patternNames = ['Chapters', 'Pages', 'Sessions', 'Episodes', 'Playlist', 'Posts', 'Memories'];

        results.forEach(resource => {
            const resultItem = document.createElement('div');
            resultItem.className = 'search-result-item';

            const typeName = typeNames[resource.type] || 'Unknown';
            const patternName = patternNames[resource.pattern] || 'Unknown';

            resultItem.innerHTML = `
                <div style="display: flex; justify-content: space-between; align-items: center;">
                    <div>
                        <div style="font-weight: 600; color: var(--text-primary); margin-bottom: 0.25rem;">
                            ${UI.escapeHtml(resource.name)}
                        </div>
                        <div style="font-size: 0.875rem; color: var(--text-muted);">
                            ${UI.escapeHtml(typeName)} • ${UI.escapeHtml(patternName)}
                        </div>
                    </div>
                </div>
            `;

            resultItem.addEventListener('click', () => {
                // Highlight selection visually
                const allItems = resultsContainer.querySelectorAll('.search-result-item');
                allItems.forEach(item => {
                    item.style.borderColor = 'var(--border-color)';
                    item.style.background = 'transparent';
                });
                resultItem.style.borderColor = 'var(--color-primary-start)';
                resultItem.style.background = 'rgba(107, 70, 193, 0.05)';

                // Update selection state
                window.currentlySelectedResource = resource;
                UI.toggleElement('confirm-add-resource-btn', true);
                
                // Ensure button is visible before scrolling
                const confirmBtn = document.getElementById('confirm-add-resource-btn');
                if (confirmBtn) {
                    confirmBtn.style.display = 'block'; // Force display just in case toggleElement is slow
                    confirmBtn.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                }
            });

            resultsContainer.appendChild(resultItem);
        });
    } catch (err) {
        if (loadingIndicator) loadingIndicator.style.display = 'none';
        console.error('Search resources failed:', err);
        resultsContainer.innerHTML = `<p style="color: var(--text-danger); text-align: center; padding: 1rem;">Search failed: ${UI.escapeHtml(err.message || 'Unknown error')}</p>`;
    }
}

async function startAssessmentPractice() {
    const subjectId = UI.getUrlParam('id');
    const roadmapId = UI.getUrlParam('roadmapId');
    const roadmapName = UI.getUrlParam('roadmapName');

    if (!roadmapId || !subjectId) {
        UI.showError('Missing roadmap or subject information');
        return;
    }

    UI.setButtonLoading('start-assessment-btn', true);

    try {
        const topics = await client.getTopics(subjectId);

        // Get assessments for assimilated topics
        const assessmentCards = [];
        for (const topic of topics) {
            try {
                const isAssimilated = await client.isAssimilated(subjectId, topic.level, topic.position);
                if (isAssimilated) {
                    const assessment = await client.getAssessments(subjectId, topic.level, topic.position);
                    if (assessment && assessment.id) {
                        assessmentCards.push({
                            id: assessment.id,
                            headline: assessment.headline,
                            state: assessment.state,
                            topicName: topic.name,
                            topicLevel: topic.level,
                            topicPosition: topic.position
                        });
                    }
                }
            } catch (err) {
                // Topic might not have an assessment yet, continue
                console.log(`No assessment for topic ${topic.name}:`, err);
            }
        }

        if (assessmentCards.length === 0) {
            UI.showError('No assessments available for practice. Create assessments in the Assessments tab first.');
            UI.setButtonLoading('start-assessment-btn', false);
            return;
        }

        // Navigate to first assessment card
        const subjectNameParam = UI.getUrlParam('name') || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        const firstAssessment = assessmentCards[0];

        window.location.href = `card.html?cardId=${firstAssessment.id}&headline=${encodeURIComponent(firstAssessment.headline)}&state=${firstAssessment.state}&practiceMode=selective&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectNameParam)}&topicName=${encodeURIComponent(firstAssessment.topicName)}&topicLevel=${firstAssessment.topicLevel}&topicPosition=${firstAssessment.topicPosition}&roadmapId=${roadmapId}&roadmapName=${encodeURIComponent(roadmapName)}`;

        UI.setButtonLoading('start-assessment-btn', false);
    } catch (err) {
        console.error('Start assessment practice failed:', err);
        UI.showError('Failed to start assessment practice: ' + (err.message || 'Unknown error'));
        UI.setButtonLoading('start-assessment-btn', false);
    }
}

async function removeTopic(level, position) {
    try {
        const subjectId = UI.getUrlParam('id');
        const subjectName = UI.getUrlParam('name');
        const roadmapId = UI.getUrlParam('roadmapId');
        const roadmapName = UI.getUrlParam('roadmapName');

        await client.removeTopic(parseInt(subjectId), level, position);
        await loadTopics();
        UI.showSuccess('Topic removed successfully');
    } catch (err) {
        console.error('Remove topic failed:', err);
        UI.showError('Failed to remove topic: ' + (err.message || 'Unknown error'));
    }
}

async function reorderTopic(level, sourcePosition, targetPosition) {
    try {
        const subjectId = UI.getUrlParam('id');
        const subjectName = UI.getUrlParam('name');
        const roadmapId = UI.getUrlParam('roadmapId');
        const roadmapName = UI.getUrlParam('roadmapName');

        await client.reorderTopic(parseInt(subjectId), level, sourcePosition, targetPosition);
        await loadTopics();
    } catch (err) {
        console.error('Reorder topic failed:', err);
        UI.showError('Failed to reorder topic: ' + (err.message || 'Unknown error'));
    }
}

// Assessment functions
async function loadAssessments() {
    UI.toggleElement('assessments-loading', true);
    UI.toggleElement('assessments-list', false);
    UI.toggleElement('assessments-empty-state', false);

    try {
        const subjectId = UI.getUrlParam('id');
        const subjectName = UI.getUrlParam('name');
        const roadmapId = UI.getUrlParam('roadmapId');
        const roadmapName = UI.getUrlParam('roadmapName');

        const topics = await client.getTopics(parseInt(subjectId));

        // Get all topics and check which are assimilated
        const topicsWithAssessments = await Promise.all(
            topics.map(async (topic) => {
                try {
                    const isAssimilated = await client.isAssimilated(subjectId, topic.level, topic.position);
                    return { ...topic, isAssimilated };
                } catch (err) {
                    return { ...topic, isAssimilated: false };
                }
            })
        );

        UI.toggleElement('assessments-loading', false);
        assessmentsLoaded = true;

        // Filter to only show assimilated topics
        const assimilatedTopics = topicsWithAssessments.filter(t => t.isAssimilated);

        if (assimilatedTopics.length === 0) {
            UI.toggleElement('assessments-empty-state', true);
        } else {
            UI.toggleElement('assessments-list', true);
            renderAssessments(assimilatedTopics);
        }
    } catch (err) {
        console.error('Loading assessments failed:', err);
        UI.toggleElement('assessments-loading', false);
        UI.showError('Failed to load assessments: ' + (err.message || 'Unknown error'));
    }
}

function renderAssessments(topics) {
    const container = document.getElementById('assessments-list');
    container.innerHTML = '';

    const levelInfo = {
        0: { name: 'Beginner', color: '#4caf50' },
        1: { name: 'Intermediate', color: '#2196f3' },
        2: { name: 'Advanced', color: '#f44336' }
    };

    topics.forEach((topic, index) => {
        const topicCard = document.createElement('div');
        topicCard.className = 'item-block';
        topicCard.style.cursor = 'default';

        const level = topic.level;
        topicCard.innerHTML = `
            <div style="width: 100%;">
                <div class="item-header">
                    <div style="display: flex; align-items: center; gap: var(--space-md); flex: 1;">
                        <span class="item-badge">${index + 1}</span>
                        <h3 class="item-title" style="margin: 0;">${UI.escapeHtml(topic.name)}</h3>
                    </div>
                    <div style="display: flex; gap: var(--space-sm); align-items: center;">
                        <span class="item-badge" style="background: rgba(102, 126, 234, 0.2); color: var(--color-primary-start);">${UI.escapeHtml(levelInfo[level].name)}</span>
                        <span class="item-badge" style="background: rgba(76, 175, 80, 0.2); color: #4caf50;">✓ Ready</span>
                    </div>
                </div>
                <div class="item-footer" style="margin-top: var(--space-md); padding-top: 0; border-top: none;">
                    <div class="item-actions" style="margin-left: 0;">
                        <button class="btn btn-sm btn-primary create-assessment-btn" data-topic-level="${topic.level}" data-topic-position="${topic.position}" data-topic-name="${UI.escapeHtml(topic.name)}">
                            Create Assessment
                        </button>
                    </div>
                </div>
            </div>
        `;

        // Create assessment button handler
        const createBtn = topicCard.querySelector('.create-assessment-btn');
        if (createBtn) {
            createBtn.addEventListener('click', async () => {
                await createAssessmentForTopic(topic.level, topic.position, topic.name);
            });
        }

        container.appendChild(topicCard);
    });
}

async function createAssessmentForTopic(topicLevel, topicPosition, topicName) {
    const headline = `Assessment: ${topicName}`;

    if (!confirm(`Create an assessment card for "${topicName}"?\n\nThis will create a comprehensive review card for this topic.`)) {
        return;
    }

    try {
        const subjectId = UI.getUrlParam('id');
        const subjectName = UI.getUrlParam('name');
        const roadmapId = UI.getUrlParam('roadmapId');
        const roadmapName = UI.getUrlParam('roadmapName');

        // First create the card
        const card = await client.createCard(headline);

        // Then create the assessment linking the card to the topic
        await client.createAssessment(card.id, subjectId, topicLevel, topicPosition);

        UI.showSuccess('Assessment created successfully! You can now add blocks to it.');

        // Navigate to the card page to edit it
        window.location.href = `card.html?cardId=${card.id}&headline=${encodeURIComponent(headline)}&state=${card.state}&practiceMode=selective&subjectId=${subjectId}&subjectName=${encodeURIComponent(subjectName || '')}&topicPosition=${topicPosition}&topicLevel=${topicLevel}&topicName=${encodeURIComponent(topicName)}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
    } catch (err) {
        console.error('Failed to create assessment:', err);
        UI.showError('Failed to create assessment: ' + (err.message || 'Unknown error'));
    }
}
