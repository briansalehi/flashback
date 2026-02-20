// Updated: 2026-02-20 with topic remove/reorder features
window.addEventListener('DOMContentLoaded', () => {
    if (!client.isAuthenticated()) {
        window.location.href = '/index.html';
        return;
    }

    const subjectId = UI.getUrlParam('id');
    const subjectName = UI.getUrlParam('name');
    const roadmapId = UI.getUrlParam('roadmapId');

    if (!subjectId) {
        window.location.href = '/home.html';
        return;
    }

    document.getElementById('subject-name').textContent = subjectName || 'Subject';
    document.title = `${subjectName || 'Subject'} - Flashback`;

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
    if (renameSubjectBtn) {
        renameSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('rename-subject-modal', true);
            document.getElementById('rename-subject-name').value = subjectName || '';
            setTimeout(() => {
                document.getElementById('rename-subject-name').focus();
            }, 100);
        });
    } else {
        console.error('Rename subject button not found');
    }

    const cancelRenameSubjectBtn = document.getElementById('cancel-rename-subject-btn');
    if (cancelRenameSubjectBtn) {
        cancelRenameSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('rename-subject-modal', false);
            UI.clearForm('rename-subject-form');
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

                UI.toggleElement('rename-subject-modal', false);
                UI.clearForm('rename-subject-form');
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
    if (removeSubjectBtn) {
        removeSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('remove-subject-modal', true);
        });
    } else {
        console.error('Remove subject button not found');
    }

    const cancelRemoveSubjectBtn = document.getElementById('cancel-remove-subject-btn');
    if (cancelRemoveSubjectBtn) {
        cancelRemoveSubjectBtn.addEventListener('click', () => {
            UI.toggleElement('remove-subject-modal', false);
        });
    }

    const confirmRemoveSubjectBtn = document.getElementById('confirm-remove-subject-btn');
    if (confirmRemoveSubjectBtn) {
        confirmRemoveSubjectBtn.addEventListener('click', async () => {
            UI.hideMessage('error-message');
            UI.setButtonLoading('confirm-remove-subject-btn', true);

            try {
                await client.removeSubject(subjectId);

                UI.toggleElement('remove-subject-modal', false);
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

            // Load data when switching tabs
            if (targetTab === 'topics' && !topicsLoaded) {
                loadTopics();
            } else if (targetTab === 'resources' && !resourcesLoaded) {
                loadResources();
            }
        });
    });

    // Topics functionality
    const addTopicBtn = document.getElementById('add-topic-btn');
    if (addTopicBtn) {
        addTopicBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-topic-form', true);
            setTimeout(() => {
                const nameInput = document.getElementById('topic-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
        });
    }

    const cancelTopicBtn = document.getElementById('cancel-topic-btn');
    if (cancelTopicBtn) {
        cancelTopicBtn.addEventListener('click', () => {
            UI.toggleElement('add-topic-form', false);
            UI.clearForm('topic-form');
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

                UI.toggleElement('add-topic-form', false);
                UI.clearForm('topic-form');
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
    const addExistingResourceBtn = document.getElementById('add-existing-resource-btn');
    if (addExistingResourceBtn) {
        addExistingResourceBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('search-resource-form', true);
            UI.toggleElement('add-resource-form', false);
            setTimeout(() => {
                const searchInput = document.getElementById('search-resource-input');
                if (searchInput) {
                    searchInput.focus();
                }
            }, 100);
        });
    }

    const createNewResourceBtn = document.getElementById('create-new-resource-btn');
    if (createNewResourceBtn) {
        createNewResourceBtn.addEventListener('click', (e) => {
            e.preventDefault();
            UI.toggleElement('add-resource-form', true);
            UI.toggleElement('search-resource-form', false);
            setTimeout(() => {
                const nameInput = document.getElementById('resource-name');
                if (nameInput) {
                    nameInput.focus();
                }
            }, 100);
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
                document.getElementById('search-resource-results').innerHTML = '<p style="color: var(--text-muted); text-align: center; padding: 1rem;">Type at least 2 characters to search...</p>';
                return;
            }

            searchTimeout = setTimeout(async () => {
                await searchAndDisplayResources(query);
            }, 300);
        });
    }

    const cancelSearchResourceBtn = document.getElementById('cancel-search-resource-btn');
    if (cancelSearchResourceBtn) {
        cancelSearchResourceBtn.addEventListener('click', () => {
            UI.toggleElement('search-resource-form', false);
            document.getElementById('search-resource-input').value = '';
            document.getElementById('search-resource-results').innerHTML = '';
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

    // Load topics initially (since topics tab is active by default)
    let topicsLoaded = false;
    let resourcesLoaded = false;
    loadTopics();
});

async function loadTopics() {
    UI.toggleElement('topics-loading', true);
    UI.toggleElement('topics-list', false);
    UI.toggleElement('topics-empty-state', false);

    try {
        const subjectId = UI.getUrlParam('id');
        const milestoneLevel = parseInt(UI.getUrlParam('level')) || 0;

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
                topicItem.className = 'topic-item';
                topicItem.draggable = true;
                topicItem.dataset.position = topic.position;
                topicItem.dataset.level = topic.level;
                topicItem.innerHTML = `
                    <div class="topic-content">
                        <div class="topic-position">${index + 1}</div>
                        <div class="topic-name">${UI.escapeHtml(topic.name)}</div>
                        <span class="topic-level">${UI.escapeHtml(levelInfo[level].name)}</span>
                    </div>
                    <div class="topic-actions">
                        <button class="topic-action-btn remove-topic-btn" data-position="${topic.position}" data-level="${topic.level}" data-name="${UI.escapeHtml(topic.name)}" title="Remove topic">×</button>
                    </div>
                `;

                // Click to navigate (but not when dragging or clicking buttons)
                let isDragging = false;
                topicItem.style.cursor = 'pointer';
                topicItem.addEventListener('click', (e) => {
                    // Don't navigate if clicking on buttons
                    if (e.target.classList.contains('topic-action-btn') ||
                        e.target.closest('.topic-action-btn')) {
                        return;
                    }

                    if (!isDragging) {
                        const subjectId = UI.getUrlParam('id');
                        const subjectName = UI.getUrlParam('name');
                        const roadmapId = UI.getUrlParam('roadmapId');
                        const roadmapName = UI.getUrlParam('roadmapName');
                        window.location.href = `topic-cards.html?subjectId=${subjectId}&topicPosition=${topic.position}&topicLevel=${topic.level}&name=${encodeURIComponent(topic.name)}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
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

                // Remove button handler
                const removeBtn = topicItem.querySelector('.remove-topic-btn');
                if (removeBtn) {
                    removeBtn.addEventListener('click', async (e) => {
                        e.stopPropagation();
                        e.preventDefault();
                        if (confirm(`Are you sure you want to remove "${topic.name}"? All cards in this topic will also be removed.`)) {
                            await removeTopic(topic.level, topic.position);
                        }
                    });
                } else {
                    console.error('Remove button not found for topic:', topic.name);
                }

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
        resourceItem.className = 'resource-item';

        // Convert epoch seconds to readable dates
        const productionDate = resource.production ? new Date(resource.production * 1000).toLocaleDateString() : 'N/A';
        const expirationDate = resource.expiration ? new Date(resource.expiration * 1000).toLocaleDateString() : 'N/A';

        const typeName = typeNames[resource.type] || 'Unknown';
        const patternName = patternNames[resource.pattern] || 'Unknown';

        resourceItem.innerHTML = `
            <div class="resource-header">
                <div style="flex: 1; cursor: pointer;" data-resource-id="${resource.id}">
                    <div class="resource-name">${UI.escapeHtml(resource.name)}</div>
                    <span class="resource-type">${UI.escapeHtml(typeName)} • ${UI.escapeHtml(patternName)}</span>
                </div>
                <button class="drop-resource-btn" data-resource-id="${resource.id}" onclick="event.stopPropagation()">
                    Drop
                </button>
            </div>
            <div class="resource-url" style="cursor: pointer;" data-resource-id="${resource.id}">
                <a href="${UI.escapeHtml(resource.link)}" target="_blank" rel="noopener noreferrer" onclick="event.stopPropagation()">
                    ${UI.escapeHtml(resource.link)}
                </a>
            </div>
            <div class="resource-dates" style="cursor: pointer;" data-resource-id="${resource.id}">
                <div><strong>Produced:</strong> ${UI.escapeHtml(productionDate)}</div>
                <div><strong>Relevant Until:</strong> ${UI.escapeHtml(expirationDate)}</div>
            </div>
        `;

        // Make most of the resource item clickable to go to resource page
        const clickableElements = resourceItem.querySelectorAll('[data-resource-id]');
        clickableElements.forEach(element => {
            if (!element.classList.contains('drop-resource-btn')) {
                element.addEventListener('click', () => {
                    const subjectId = UI.getUrlParam('id');
                    const subjectName = UI.getUrlParam('name');
                    const roadmapId = UI.getUrlParam('roadmapId');
                    const roadmapName = UI.getUrlParam('roadmapName');
                    window.location.href = `resource.html?id=${resource.id}&name=${encodeURIComponent(resource.name)}&subjectId=${subjectId || ''}&subjectName=${encodeURIComponent(subjectName || '')}&roadmapId=${roadmapId || ''}&roadmapName=${encodeURIComponent(roadmapName || '')}`;
                });
            }
        });

        // Add drop button handler
        const dropBtn = resourceItem.querySelector('.drop-resource-btn');
        dropBtn.addEventListener('click', async (e) => {
            e.stopPropagation();
            if (confirm(`Are you sure you want to drop "${resource.name}" from this subject?`)) {
                try {
                    const subjectId = UI.getUrlParam('id');
                    await client.dropResourceFromSubject(resource.id, parseInt(subjectId));
                    loadResources(); // Reload the list
                } catch (err) {
                    console.error('Drop resource failed:', err);
                    UI.showError('Failed to drop resource: ' + (err.message || 'Unknown error'));
                }
            }
        });

        container.appendChild(resourceItem);
    });
}

async function startPracticeMode() {
    const subjectId = UI.getUrlParam('id');
    const roadmapId = UI.getUrlParam('roadmapId');

    if (!roadmapId || !subjectId) {
        UI.showError('Missing roadmap or subject information');
        return;
    }

    UI.setButtonLoading('start-practice-btn', true);

    try {
        const topics = await client.getPracticeTopics(parseInt(roadmapId), parseInt(subjectId));

        if (topics.length === 0) {
            UI.showError('No topics available for practice');
            UI.setButtonLoading('start-practice-btn', false);
            return;
        }

        // Store practice state in sessionStorage
        sessionStorage.setItem('practiceState', JSON.stringify({
            roadmapId: parseInt(roadmapId),
            subjectId: parseInt(subjectId),
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
        const subjectNameParam = UI.getUrlParam('name') || '';
        const roadmapName = UI.getUrlParam('roadmapName') || '';
        window.location.href = `card.html?cardId=${cards[0].id}&headline=${encodeURIComponent(cards[0].headline)}&state=${cards[0].state}&practiceMode=aggressive&cardIndex=0&totalCards=${cards.length}&subjectName=${encodeURIComponent(subjectNameParam)}&topicName=${encodeURIComponent(firstTopicWithCards.name)}&roadmapName=${encodeURIComponent(roadmapName)}`;
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
        const roadmapName = UI.getUrlParam('roadmapName');
        if (roadmapName) {
            breadcrumb.innerHTML = `<a href="roadmap.html?id=${roadmapId}&name=${encodeURIComponent(roadmapName)}" style="color: var(--text-primary); text-decoration: none;">${UI.escapeHtml(roadmapName)}</a>`;
        }
    } catch (err) {
        console.error('Failed to display breadcrumb:', err);
    }
}

async function searchAndDisplayResources(query) {
    const resultsContainer = document.getElementById('search-resource-results');
    resultsContainer.innerHTML = '<p style="color: var(--text-muted); text-align: center; padding: 1rem;">Searching...</p>';

    try {
        const results = await client.searchResources(query);

        if (results.length === 0) {
            resultsContainer.innerHTML = '<p style="color: var(--text-muted); text-align: center; padding: 1rem;">No resources found.</p>';
            return;
        }

        resultsContainer.innerHTML = '';
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

            resultItem.addEventListener('click', async () => {
                try {
                    const subjectId = UI.getUrlParam('id');
                    await client.addResourceToSubject(parseInt(subjectId), resource.id);

                    UI.toggleElement('search-resource-form', false);
                    document.getElementById('search-resource-input').value = '';
                    resultsContainer.innerHTML = '';

                    loadResources(); // Reload the resources list
                    UI.showSuccess(`Resource "${resource.name}" added successfully`);
                } catch (err) {
                    console.error('Add resource failed:', err);
                    UI.showError('Failed to add resource: ' + (err.message || 'Unknown error'));
                }
            });

            resultsContainer.appendChild(resultItem);
        });
    } catch (err) {
        console.error('Search resources failed:', err);
        resultsContainer.innerHTML = `<p style="color: var(--text-danger); text-align: center; padding: 1rem;">Search failed: ${UI.escapeHtml(err.message || 'Unknown error')}</p>`;
    }
}

async function removeTopic(level, position) {
    const subjectId = UI.getUrlParam('id');

    try {
        await client.removeTopic(parseInt(subjectId), level, position);
        await loadTopics();
        UI.showSuccess('Topic removed successfully');
    } catch (err) {
        console.error('Remove topic failed:', err);
        UI.showError('Failed to remove topic: ' + (err.message || 'Unknown error'));
    }
}

async function reorderTopic(level, sourcePosition, targetPosition) {
    const subjectId = UI.getUrlParam('id');

    try {
        await client.reorderTopic(parseInt(subjectId), level, sourcePosition, targetPosition);
        await loadTopics();
    } catch (err) {
        console.error('Reorder topic failed:', err);
        UI.showError('Failed to reorder topic: ' + (err.message || 'Unknown error'));
    }
}
