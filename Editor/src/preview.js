class PreviewEngine {
    constructor() {
        this.canvas = document.getElementById('previewCanvas');
        this.ctx = this.canvas.getContext('2d');
        this.isPlaying = false;
        this.currentTime = 0;
        this.frameRate = 30;
        this.animationId = null;
        
        this.setupControls();
        this.drawPlaceholder();
    }
    
    setupControls() {
        document.getElementById('playBtn').addEventListener('click', () => this.play());
        document.getElementById('pauseBtn').addEventListener('click', () => this.pause());
        document.getElementById('stopBtn').addEventListener('click', () => this.stop());
    }
    
    drawPlaceholder() {
        this.ctx.fillStyle = '#000';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        
        this.ctx.fillStyle = '#667eea';
        this.ctx.font = '48px Arial';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('Preview', this.canvas.width / 2, this.canvas.height / 2 - 20);
        
        this.ctx.fillStyle = '#999';
        this.ctx.font = '24px Arial';
        this.ctx.fillText('Add clips to timeline to preview', this.canvas.width / 2, this.canvas.height / 2 + 30);
    }
    
    play() {
        if (this.isPlaying) return;
        if (!window.timeline || window.timeline.clips.length === 0) {
            alert('Add video clips to the timeline first!');
            return;
        }
        
        this.isPlaying = true;
        this.animate();
    }
    
    pause() {
        this.isPlaying = false;
        if (this.animationId) {
            cancelAnimationFrame(this.animationId);
        }
    }
    
    stop() {
        this.pause();
        this.currentTime = 0;
        window.timeline.playhead = 0;
        window.timeline.updatePlayheadPosition();
        this.updateTimeDisplay();
    }
    
    seekTo(time) {
        this.currentTime = time;
        window.timeline.playhead = time;
        this.renderFrame();
        this.updateTimeDisplay();
    }
    
    animate() {
        if (!this.isPlaying) return;
        
        this.currentTime += 1 / this.frameRate;
        
        if (this.currentTime > window.timeline.duration) {
            this.stop();
            return;
        }
        
        window.timeline.playhead = this.currentTime;
        window.timeline.updatePlayheadPosition();
        
        this.renderFrame();
        this.updateTimeDisplay();
        
        this.animationId = requestAnimationFrame(() => this.animate());
    }
    
    renderFrame() {
        // Clear canvas
        this.ctx.fillStyle = '#000';
        this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Find active clips at current time
        const activeClips = window.timeline.clips.filter(clip => {
            return this.currentTime >= clip.timelinePosition && 
                   this.currentTime < clip.timelinePosition + clip.duration;
        });
        
        if (activeClips.length === 0) {
            this.ctx.fillStyle = '#333';
            this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
            return;
        }
        
        // Draw each active clip
        activeClips.forEach(clip => {
            const relativeTime = this.currentTime - clip.timelinePosition;
            const sourceTime = clip.startTime + relativeTime;
            
            // Draw video frame (simplified - in real app, decode actual video)
            this.ctx.fillStyle = '#667eea';
            this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
            
            // Draw clip info
            this.ctx.fillStyle = '#fff';
            this.ctx.font = '32px Arial';
            this.ctx.textAlign = 'center';
            this.ctx.fillText(clip.media.name, this.canvas.width / 2, this.canvas.height / 2);
            
            this.ctx.font = '20px Arial';
            this.ctx.fillText(`Time: ${sourceTime.toFixed(2)}s`, this.canvas.width / 2, this.canvas.height / 2 + 40);
        });
        
        // Note: In a real editor, you would:
        // 1. Decode video frame from clip.media.video element
        // 2. Apply effects using canvas filters or WebGL shaders
        // 3. Composite multiple tracks
        // 4. Render text overlays
    }
    
    updateTimeDisplay() {
        const current = this.formatTime(this.currentTime);
        const total = this.formatTime(window.timeline.duration);
        document.getElementById('timeDisplay').textContent = `${current} / ${total}`;
    }
    
    formatTime(seconds) {
        const mins = Math.floor(seconds / 60);
        const secs = Math.floor(seconds % 60);
        return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
    }
}

// Initialize preview engine
window.preview = new PreviewEngine();