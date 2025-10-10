// Timeline Management with Full Effects Support
class Timeline {
    constructor() {
        this.clips = [];
        this.textLayers = [];
        this.currentTime = 0;
        this.duration = 0;
        this.pixelsPerSecond = 50;
        this.selectedClip = null;
        this.playhead = 0;
        
        this.setupEventListeners();
    }
    
    setupEventListeners() {
        const ruler = document.getElementById('timelineRuler');
        ruler.addEventListener('click', (e) => {
            const rect = ruler.getBoundingClientRect();
            const x = e.clientX - rect.left;
            this.playhead = x / this.pixelsPerSecond;
            this.updatePlayheadPosition();
            if (window.preview) {
                window.preview.seekTo(this.playhead);
            }
        });
        
        const tracks = document.querySelectorAll('.timeline-track');
        tracks.forEach(track => {
            track.addEventListener('dragover', (e) => {
                e.preventDefault();
                track.style.background = '#4d4d4d';
            });
            
            track.addEventListener('dragleave', () => {
                track.style.background = '#3d3d3d';
            });
            
            track.addEventListener('drop', (e) => {
                e.preventDefault();
                track.style.background = '#3d3d3d';
                
                const mediaId = e.dataTransfer.getData('mediaId');
                const media = window.mediaLibrary.find(m => m.id === mediaId);
                
                if (media) {
                    const rect = track.getBoundingClientRect();
                    const x = e.clientX - rect.left;
                    const timePosition = x / this.pixelsPerSecond;
                    
                    this.addClip(media, timePosition, track.dataset.track);
                }
            });
        });
    }
    
    addClip(media, timePosition, trackType) {
        const clip = {
            id: 'clip_' + Date.now(),
            media: media,
            startTime: 0,
            endTime: media.duration || 10,
            timelinePosition: timePosition,
            track: trackType,
            duration: media.duration || 10,
            effects: {
                brightness: 0,
                contrast: 1,
                saturation: 1,
                blur: 0,
                sharpen: 0,
                volume: 1,
                rotate: 0,
                fadeIn: 0,
                fadeOut: 0
            }
        };
        
        this.clips.push(clip);
        this.renderClip(clip);
        this.updateDuration();
        
        if (window.editorModule) {
            window.editorModule.ccall('add_video_clip', 'number',
                ['string', 'number', 'number', 'number', 'number'],
                [media.filename, clip.startTime, clip.endTime, clip.timelinePosition, 0]
            );
        }
        
        document.getElementById('exportBtn').disabled = false;
    }
    
    renderClip(clip) {
        const track = document.querySelector(`[data-track="${clip.track}"]`);
        if (!track) return;
        
        const clipEl = document.createElement('div');
        clipEl.className = 'timeline-clip';
        clipEl.id = clip.id;
        clipEl.style.left = (clip.timelinePosition * this.pixelsPerSecond) + 'px';
        clipEl.style.width = (clip.duration * this.pixelsPerSecond) + 'px';
        clipEl.textContent = clip.media.name;
        clipEl.draggable = true;
        
        clipEl.addEventListener('click', (e) => {
            e.stopPropagation();
            this.selectClip(clip);
        });
        
        clipEl.addEventListener('dragstart', (e) => {
            e.dataTransfer.setData('clipId', clip.id);
        });
        
        track.appendChild(clipEl);
    }
    
    selectClip(clip) {
        if (this.selectedClip) {
            const prevEl = document.getElementById(this.selectedClip.id);
            if (prevEl) prevEl.classList.remove('selected');
        }
        
        this.selectedClip = clip;
        const clipEl = document.getElementById(clip.id);
        if (clipEl) clipEl.classList.add('selected');
        
        this.showProperties(clip);
    }
    
    showProperties(clip) {
        const panel = document.getElementById('propertiesContent');
        const effects = clip.effects;
        
        panel.innerHTML = `
            <h4 style="color: #667eea; margin-bottom: 15px;">📹 ${clip.media.name}</h4>
            
            <!-- TIMING -->
            <div class="property-group">
                <label>⏱️ Trim Start (s):</label>
                <input type="number" id="clipStartTime" value="${clip.startTime}" step="0.1" min="0" max="${clip.media.duration}">
            </div>
            <div class="property-group">
                <label>⏱️ Trim End (s):</label>
                <input type="number" id="clipEndTime" value="${clip.endTime}" step="0.1" min="0" max="${clip.media.duration}">
            </div>
            <div class="property-group">
                <label>📍 Timeline Position (s):</label>
                <input type="number" id="clipTimelinePos" value="${clip.timelinePosition}" step="0.1" min="0">
            </div>
            
            <!-- COLOR & FILTERS -->
            <h4 style="color: #667eea; margin: 20px 0 15px 0;">🎨 Color & Filters</h4>
            <div class="property-group">
                <label>☀️ Brightness (-1 to 1): <span id="brightnessVal">${effects.brightness}</span></label>
                <input type="range" id="brightness" min="-1" max="1" step="0.1" value="${effects.brightness}" 
                       oninput="document.getElementById('brightnessVal').textContent = this.value">
            </div>
            <div class="property-group">
                <label>🔆 Contrast (0 to 2): <span id="contrastVal">${effects.contrast}</span></label>
                <input type="range" id="contrast" min="0" max="2" step="0.1" value="${effects.contrast}"
                       oninput="document.getElementById('contrastVal').textContent = this.value">
            </div>
            <div class="property-group">
                <label>🌈 Saturation (0 to 2): <span id="saturationVal">${effects.saturation}</span></label>
                <input type="range" id="saturation" min="0" max="2" step="0.1" value="${effects.saturation}"
                       oninput="document.getElementById('saturationVal').textContent = this.value">
            </div>
            <div class="property-group">
                <label>🌀 Blur (0 to 10): <span id="blurVal">${effects.blur}</span></label>
                <input type="range" id="blur" min="0" max="10" step="1" value="${effects.blur}"
                       oninput="document.getElementById('blurVal').textContent = this.value">
            </div>
            <div class="property-group">
                <label>✨ Sharpen (0 to 5): <span id="sharpenVal">${effects.sharpen}</span></label>
                <input type="range" id="sharpen" min="0" max="5" step="0.5" value="${effects.sharpen}"
                       oninput="document.getElementById('sharpenVal').textContent = this.value">
            </div>
            
            <!-- TRANSFORM -->
            <h4 style="color: #667eea; margin: 20px 0 15px 0;">🔄 Transform</h4>
            <div class="property-group">
                <label>🔄 Rotate (degrees):</label>
                <input type="number" id="rotate" value="${effects.rotate || 0}" step="45" min="-360" max="360">
            </div>
            <div class="property-group">
                <label>↔️ Flip:</label>
                <select id="flip">
                    <option value="">None</option>
                    <option value="horizontal" ${effects.flip === 'horizontal' ? 'selected' : ''}>Horizontal</option>
                    <option value="vertical" ${effects.flip === 'vertical' ? 'selected' : ''}>Vertical</option>
                </select>
            </div>
            <div class="property-group">
                <button onclick="window.timeline.showCropDialog()">✂️ Crop Video</button>
            </div>
            <div class="property-group">
                <button onclick="window.timeline.showScaleDialog()">📐 Resize Video</button>
            </div>
            
            <!-- AUDIO -->
            <h4 style="color: #667eea; margin: 20px 0 15px 0;">🔊 Audio</h4>
            <div class="property-group">
                <label>🔊 Volume (0 to 2): <span id="volumeVal">${effects.volume}</span></label>
                <input type="range" id="volume" min="0" max="2" step="0.1" value="${effects.volume}"
                       oninput="document.getElementById('volumeVal').textContent = this.value">
            </div>
            <div class="property-group">
                <button onclick="window.timeline.muteClip()" style="background: #dc3545;">🔇 Mute Audio</button>
            </div>
            
            <!-- TRANSITIONS -->
            <h4 style="color: #667eea; margin: 20px 0 15px 0;">✨ Transitions</h4>
            <div class="property-group">
                <label>🌅 Fade In (seconds):</label>
                <input type="number" id="fadeIn" value="${effects.fadeIn || 0}" step="0.5" min="0" max="5">
            </div>
            <div class="property-group">
                <label>🌃 Fade Out (seconds):</label>
                <input type="number" id="fadeOut" value="${effects.fadeOut || 0}" step="0.5" min="0" max="5">
            </div>
            
            <!-- ACTIONS -->
            <div class="property-group" style="margin-top: 25px;">
                <button onclick="window.timeline.applyEffects()" style="background: #28a745; font-size: 16px;">
                    ✅ Apply All Effects
                </button>
            </div>
            <div class="property-group">
                <button onclick="window.timeline.resetEffects()" style="background: #ffc107;">
                    🔄 Reset Effects
                </button>
            </div>
            <div class="property-group">
                <button onclick="window.timeline.deleteClip()" style="background: #dc3545;">
                    🗑️ Delete Clip
                </button>
            </div>
        `;
    }
    
    applyEffects() {
        if (!this.selectedClip) return;
        
        const clip = this.selectedClip;
        
        // Update timing
        clip.startTime = parseFloat(document.getElementById('clipStartTime').value);
        clip.endTime = parseFloat(document.getElementById('clipEndTime').value);
        clip.timelinePosition = parseFloat(document.getElementById('clipTimelinePos').value);
        clip.duration = clip.endTime - clip.startTime;
        
        // Update effects
        clip.effects.brightness = parseFloat(document.getElementById('brightness').value);
        clip.effects.contrast = parseFloat(document.getElementById('contrast').value);
        clip.effects.saturation = parseFloat(document.getElementById('saturation').value);
        clip.effects.blur = parseFloat(document.getElementById('blur').value);
        clip.effects.sharpen = parseFloat(document.getElementById('sharpen').value);
        clip.effects.volume = parseFloat(document.getElementById('volume').value);
        clip.effects.rotate = parseFloat(document.getElementById('rotate').value);
        clip.effects.flip = document.getElementById('flip').value;
        clip.effects.fadeIn = parseFloat(document.getElementById('fadeIn').value);
        clip.effects.fadeOut = parseFloat(document.getElementById('fadeOut').value);
        
        // Update visual
        const clipEl = document.getElementById(clip.id);
        if (clipEl) {
            clipEl.style.left = (clip.timelinePosition * this.pixelsPerSecond) + 'px';
            clipEl.style.width = (clip.duration * this.pixelsPerSecond) + 'px';
        }
        
        this.updateDuration();
        
        alert('✅ Effects applied! Click "Export Video" to see results.');
    }
    
    resetEffects() {
        if (!this.selectedClip) return;
        
        this.selectedClip.effects = {
            brightness: 0,
            contrast: 1,
            saturation: 1,
            blur: 0,
            sharpen: 0,
            volume: 1,
            rotate: 0,
            fadeIn: 0,
            fadeOut: 0
        };
        
        this.showProperties(this.selectedClip);
        alert('🔄 Effects reset to default!');
    }
    
    muteClip() {
        if (!this.selectedClip) return;
        this.selectedClip.effects.volume = 0;
        document.getElementById('volume').value = 0;
        document.getElementById('volumeVal').textContent = '0';
        alert('🔇 Audio muted!');
    }
    
    showCropDialog() {
        if (!this.selectedClip) return;
        
        const width = prompt('Crop Width (pixels):', this.selectedClip.media.width);
        const height = prompt('Crop Height (pixels):', this.selectedClip.media.height);
        const x = prompt('Crop X position:', '0');
        const y = prompt('Crop Y position:', '0');
        
        if (width && height) {
            this.selectedClip.effects.crop = {
                width: parseInt(width),
                height: parseInt(height),
                x: parseInt(x) || 0,
                y: parseInt(y) || 0
            };
            alert(`✂️ Crop set to ${width}x${height} at (${x},${y})`);
        }
    }
    
    showScaleDialog() {
        if (!this.selectedClip) return;
        
        const width = prompt('New Width (pixels):', '1920');
        const height = prompt('New Height (pixels):', '1080');
        
        if (width && height) {
            this.selectedClip.effects.scale = {
                width: parseInt(width),
                height: parseInt(height)
            };
            alert(`📐 Scale set to ${width}x${height}`);
        }
    }
    
    deleteClip() {
        if (!this.selectedClip) return;
        
        const clipEl = document.getElementById(this.selectedClip.id);
        if (clipEl) clipEl.remove();
        
        this.clips = this.clips.filter(c => c.id !== this.selectedClip.id);
        this.selectedClip = null;
        
        document.getElementById('propertiesContent').innerHTML = 
            '<p style="color: #999;">Select a clip to edit properties</p>';
        
        if (this.clips.length === 0) {
            document.getElementById('exportBtn').disabled = true;
        }
        
        this.updateDuration();
    }
    
    updateDuration() {
        this.duration = 0;
        this.clips.forEach(clip => {
            const end = clip.timelinePosition + clip.duration;
            if (end > this.duration) {
                this.duration = end;
            }
        });
    }
    
    updatePlayheadPosition() {
        const marker = document.getElementById('playheadMarker');
        marker.style.left = (this.playhead * this.pixelsPerSecond) + 'px';
    }
    
    addTextLayer() {
        const text = prompt('Enter text:');
        if (!text) return;
        
        const x = prompt('X position (pixels):', '100');
        const y = prompt('Y position (pixels):', '100');
        const fontSize = prompt('Font size:', '48');
        const color = prompt('Color (e.g., white, red, #FF0000):', 'white');
        
        const textLayer = {
            id: 'text_' + Date.now(),
            text: text,
            x: parseInt(x) || 100,
            y: parseInt(y) || 100,
            fontSize: parseInt(fontSize) || 48,
            color: color || 'white',
            startTime: this.playhead,
            endTime: this.playhead + 5
        };
        
        this.textLayers.push(textLayer);
        
        if (window.editorModule) {
            window.editorModule.ccall('add_text_layer', 'number',
                ['string', 'number', 'number', 'number', 'string', 'number', 'number'],
                [text, textLayer.x, textLayer.y, textLayer.fontSize, textLayer.color, textLayer.startTime, textLayer.endTime]
            );
        }
        
        const track = document.querySelector('[data-track="text"]');
        const textEl = document.createElement('div');
        textEl.className = 'timeline-clip';
        textEl.style.left = (textLayer.startTime * this.pixelsPerSecond) + 'px';
        textEl.style.width = ((textLayer.endTime - textLayer.startTime) * this.pixelsPerSecond) + 'px';
        textEl.style.background = 'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)';
        textEl.textContent = text;
        track.appendChild(textEl);
        
        alert(`📝 Text added at ${textLayer.startTime.toFixed(2)}s for 5 seconds!`);
    }
}

window.timeline = new Timeline();