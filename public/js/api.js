// LibroSphere API Client

const API = {
    getToken() {
        return localStorage.getItem('librosphere_token') || '';
    },

    setToken(token) {
        if (token) {
            localStorage.setItem('librosphere_token', token);
        } else {
            localStorage.removeItem('librosphere_token');
        }
    },

    getUser() {
        try {
            const raw = localStorage.getItem('librosphere_user');
            return raw ? JSON.parse(raw) : null;
        } catch (e) {
            return null;
        }
    },

    setUser(user) {
        if (user) {
            localStorage.setItem('librosphere_user', JSON.stringify(user));
        } else {
            localStorage.removeItem('librosphere_user');
        }
    },

    async request(endpoint, options = {}) {
        const url = endpoint.startsWith('http') ? endpoint : endpoint;
        const headers = {
            'Content-Type': 'application/json',
            ...(options.headers || {})
        };

        const token = this.getToken();
        if (token) {
            headers['Authorization'] = `Bearer ${token}`;
            headers['X-Session-Token'] = token;
        }

        const config = {
            ...options,
            headers
        };

        try {
            const res = await fetch(url, config);
            const data = await res.json().catch(() => ({}));

            if (!res.ok) {
                const msg = data.error || data.message || `Request failed (${res.status})`;
                throw new Error(msg);
            }

            return data;
        } catch (err) {
            console.error(`API Error [${endpoint}]:`, err);
            throw err;
        }
    },

    get(endpoint) {
        return this.request(endpoint, { method: 'GET' });
    },

    post(endpoint, body) {
        return this.request(endpoint, {
            method: 'POST',
            body: JSON.stringify(body)
        });
    },

    put(endpoint, body) {
        return this.request(endpoint, {
            method: 'PUT',
            body: JSON.stringify(body)
        });
    },

    delete(endpoint) {
        return this.request(endpoint, { method: 'DELETE' });
    }
};

// Toast Notifications
function showToast(message, type = 'info') {
    let container = document.querySelector('.toast-container');
    if (!container) {
        container = document.createElement('div');
        container.className = 'toast-container';
        document.body.appendChild(container);
    }

    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    let icon = 'ℹ️';
    if (type === 'success') icon = '✅';
    if (type === 'error') icon = '❌';

    toast.innerHTML = `<span>${icon}</span> <span>${message}</span>`;
    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateY(10px)';
        toast.style.transition = 'all 0.3s ease';
        setTimeout(() => toast.remove(), 300);
    }, 3500);
}
