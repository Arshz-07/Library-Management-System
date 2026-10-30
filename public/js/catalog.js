// LibroSphere Catalog Logic

let currentCategoryId = 0;
let currentSort = 'title';
let currentOnlyAvailable = false;
let currentSearchQuery = '';

const Catalog = {
    async init() {
        await this.loadStats();
        await this.loadCategories();
        await this.loadBooks();
        this.bindEvents();
    },

    async loadStats() {
        try {
            const res = await API.get('/api/dashboard/stats');
            if (res.success && res.data) {
                const s = res.data;
                const setVal = (id, val) => {
                    const el = document.getElementById(id);
                    if (el) el.innerText = val.toLocaleString();
                };
                setVal('stat-total-titles', s.totalTitles || 0);
                setVal('stat-total-copies', s.totalCopies || 0);
                setVal('stat-available-copies', s.availableCopies || 0);
                setVal('stat-total-members', s.totalMembers || 0);
            }
        } catch (e) {
            console.warn('Failed to load summary stats', e);
        }
    },

    async loadCategories() {
        const container = document.getElementById('category-pills-container');
        if (!container) return;

        try {
            const res = await API.get('/api/categories');
            if (res.success && res.data && res.data.categories) {
                const cats = res.data.categories;
                let html = `
                    <button class="category-pill active" data-cat-id="0">
                        🌟 All Genres
                    </button>
                `;
                cats.forEach(c => {
                    html += `
                        <button class="category-pill" data-cat-id="${c.id}">
                            ${c.icon || '📚'} ${c.name} (${c.bookCount})
                        </button>
                    `;
                });
                container.innerHTML = html;

                container.querySelectorAll('.category-pill').forEach(btn => {
                    btn.addEventListener('click', (e) => {
                        container.querySelectorAll('.category-pill').forEach(b => b.classList.remove('active'));
                        btn.classList.add('active');
                        currentCategoryId = parseInt(btn.getAttribute('data-cat-id'), 10) || 0;
                        this.loadBooks();
                    });
                });
            }
        } catch (err) {
            console.error('Error loading categories:', err);
        }
    },

    async loadBooks() {
        const grid = document.getElementById('book-grid-container');
        const countEl = document.getElementById('catalog-results-count');
        if (!grid) return;

        grid.innerHTML = `
            <div style="grid-column: 1 / -1; text-align: center; padding: 4rem 1rem; color: var(--text-muted);">
                <div style="font-size: 2rem; margin-bottom: 0.5rem;">📖</div>
                <p>Loading library collection...</p>
            </div>
        `;

        const params = new URLSearchParams();
        if (currentSearchQuery) params.append('q', currentSearchQuery);
        if (currentCategoryId > 0) params.append('category', currentCategoryId);
        if (currentOnlyAvailable) params.append('available', 'true');
        if (currentSort) params.append('sort', currentSort);

        try {
            const res = await API.get(`/api/books?${params.toString()}`);
            if (res.success && res.data && res.data.books) {
                const books = res.data.books;
                if (countEl) countEl.innerText = `${books.length} book${books.length === 1 ? '' : 's'} found`;
                this.renderBooks(books);
            }
        } catch (err) {
            grid.innerHTML = `
                <div style="grid-column: 1 / -1; text-align: center; padding: 3rem; color: var(--danger);">
                    <p>Failed to load books: ${err.message}</p>
                </div>
            `;
        }
    },

    renderBooks(books) {
        const grid = document.getElementById('book-grid-container');
        if (!grid) return;

        if (books.length === 0) {
            grid.innerHTML = `
                <div style="grid-column: 1 / -1; text-align: center; padding: 4rem 1rem; color: var(--text-muted); background: white; border-radius: var(--radius-lg); border: 1px dashed var(--border-light);">
                    <div style="font-size: 3rem; margin-bottom: 0.5rem;">🔍</div>
                    <h3 style="font-size: 1.25rem; font-weight: 700; color: var(--text-main); margin-bottom: 0.5rem;">No matching volumes found</h3>
                    <p>Try clearing your search query or switching categories.</p>
                </div>
            `;
            return;
        }

        const user = Auth.getCurrentUser();

        grid.innerHTML = books.map(b => {
            const isAvail = b.availableCopies > 0;
            const stockBadge = isAvail
                ? `<span class="book-badge-stock stock-in">✓ ${b.availableCopies} of ${b.totalCopies} Available</span>`
                : `<span class="book-badge-stock stock-out">Out of Stock</span>`;

            const fallbackCover = 'https://images.unsplash.com/photo-1544716278-ca5e3f4abd8c?w=500&auto=format&fit=crop&q=80';
            const cover = b.coverUrl || fallbackCover;

            return `
                <div class="book-card" data-id="${b.id}">
                    <div class="book-cover-wrap">
                        <img src="${cover}" alt="${b.title}" class="book-cover-img" loading="lazy" onerror="this.src='${fallbackCover}'">
                        <span class="book-badge-category">${b.categoryName || 'Book'}</span>
                        ${stockBadge}
                    </div>
                    <div class="book-info">
                        <h3 class="book-title" title="${b.title}">${b.title}</h3>
                        <p class="book-author">by ${b.author}</p>
                        
                        <div class="book-meta">
                            <span class="book-shelf">📍 ${b.shelfLocation || 'Main Stack'}</span>
                            <span>&bull; ${b.publishYear}</span>
                        </div>

                        <div class="book-card-actions">
                            <button class="btn btn-secondary btn-sm" style="flex: 1;" onclick="Catalog.openBookModal(${b.id})">
                                Details
                            </button>
                            ${isAvail ? `
                                <button class="btn btn-primary btn-sm" onclick="Catalog.borrowBook(${b.id})">
                                    Borrow
                                </button>
                            ` : `
                                <button class="btn btn-secondary btn-sm" disabled style="opacity: 0.5; cursor: not-allowed;">
                                    Reserved
                                </button>
                            `}
                        </div>
                    </div>
                </div>
            `;
        }).join('');
    },

    bindEvents() {
        const searchInput = document.getElementById('search-books-input');
        const heroInput = document.getElementById('hero-search-input');
        const sortSelect = document.getElementById('sort-books-select');
        const availToggle = document.getElementById('filter-available-toggle');

        let timeout = null;
        const onSearch = (val) => {
            clearTimeout(timeout);
            timeout = setTimeout(() => {
                currentSearchQuery = val.trim();
                this.loadBooks();
            }, 300);
        };

        if (searchInput) {
            searchInput.addEventListener('input', (e) => onSearch(e.target.value));
        }

        if (heroInput) {
            heroInput.addEventListener('input', (e) => {
                if (searchInput) searchInput.value = e.target.value;
                onSearch(e.target.value);
            });
            heroInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') {
                    document.getElementById('catalog-section')?.scrollIntoView({ behavior: 'smooth' });
                }
            });
        }

        if (sortSelect) {
            sortSelect.addEventListener('change', (e) => {
                currentSort = e.target.value;
                this.loadBooks();
            });
        }

        if (availToggle) {
            availToggle.addEventListener('change', (e) => {
                currentOnlyAvailable = e.target.checked;
                this.loadBooks();
            });
        }
    },

    async openBookModal(id) {
        try {
            const res = await API.get(`/api/books/${id}`);
            if (res.success && res.data && res.data.book) {
                const b = res.data.book;
                const modal = document.getElementById('book-detail-modal');
                if (!modal) return;

                document.getElementById('modal-book-title').innerText = b.title;
                document.getElementById('modal-book-author').innerText = `by ${b.author}`;
                document.getElementById('modal-book-category').innerText = b.categoryName;
                document.getElementById('modal-book-isbn').innerText = b.isbn;
                document.getElementById('modal-book-publisher').innerText = `${b.publisher || 'Unknown'} (${b.publishYear})`;
                document.getElementById('modal-book-shelf').innerText = b.shelfLocation || 'General Stack';
                document.getElementById('modal-book-copies').innerText = `${b.availableCopies} available of ${b.totalCopies} total copies`;
                document.getElementById('modal-book-desc').innerText = b.description || 'No detailed synopsis provided for this volume.';
                
                const cover = document.getElementById('modal-book-cover');
                if (cover) cover.src = b.coverUrl || 'https://images.unsplash.com/photo-1544716278-ca5e3f4abd8c?w=500&auto=format&fit=crop&q=80';

                const borrowBtn = document.getElementById('modal-borrow-btn');
                if (borrowBtn) {
                    if (b.availableCopies > 0) {
                        borrowBtn.style.display = 'inline-flex';
                        borrowBtn.onclick = () => this.borrowBook(b.id);
                    } else {
                        borrowBtn.style.display = 'none';
                    }
                }

                modal.classList.add('show');
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    },

    closeBookModal() {
        const modal = document.getElementById('book-detail-modal');
        if (modal) modal.classList.remove('show');
    },

    async borrowBook(bookId) {
        if (!Auth.isLoggedIn()) {
            showToast('Please sign in or create an account to borrow books.', 'info');
            setTimeout(() => { window.location.href = '/login.html'; }, 1000);
            return;
        }

        try {
            const res = await API.post('/api/loans/issue', {
                bookId: bookId,
                loanDays: 14,
                notes: 'Borrowed via Web Catalog'
            });

            if (res.success) {
                showToast('Book borrowed successfully! Due in 14 days.', 'success');
                this.closeBookModal();
                await this.loadBooks();
                await this.loadStats();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    }
};

document.addEventListener('DOMContentLoaded', () => {
    Catalog.init();
});
