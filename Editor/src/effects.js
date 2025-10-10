// Media Library & Export Management - FULL FEATURED VERSION
let mediaLibrary = [];
let ffmpeg = null;
let editorModule = null;

// Initialize when page loads
document.addEventListener('DOMContentLoaded', () => {
    setupMediaLibrary();
    setupExport();
    loadEditorModule();
});

function setupMediaLibrary() {
    const uploadZone = document.getElementById('uploadZone');
    const mediaInput = document.getElementById('mediaInput');
    
    uploadZone.addEventListener('click', () => mediaInput.click());
    
    uploadZone.addEventListener('dragover', (e) => {
        e.preventDefault();
        uploadZone.style.background = 'rgba(102, 126, 234, 0.2)';
    });
    
    uploadZone.addEventListener('dragleave', () => {
        uploadZone.style.background = '';
    });
    
    uploadZone.addEventListener('drop', (e) => {
        e.preventDefault();
        uploadZone.style.background = '';
        handleFiles(e.dataTransfer.files);
    });
    
    mediaInput.addEventListener('change', (e) => {
        handleFiles(e.target.files);
    });
}

function handleFiles(files) {
    Array.from(files).forEach(file => {
        if (file.type.startsWith('video/')) {
            addMediaToLibrary(file);
        }
    });
}

function addMediaToLibrary(file) {
    const video = document.createElement('video');
    video.preload = 'metadata';
    video.src = URL.createObjectURL(file);
    
    video.addEventListener('loadedmetadata', () => {
        const media = {
            id: 'media_' + Date.now() + Math.random(),
            name: file.name,
            filename: file.name,
            file: file,
            video: video,
            duration: video.duration,
            width: video.videoWidth,
            height: video.videoHeight
        };
        
        mediaLibrary.push(media);
        window.mediaLibrary = mediaLibrary;
        renderMediaItem(media);
    });
}

function renderMediaItem(media) {
    const mediaList = document.getElementById('mediaList');
    const item = document.createElement('div');
    item.className = 'media-item';
    item.draggable = true;
    
    item.innerHTML = `
        <video></video>
        <div class="media-item-name">${media.name}</div>
        <div class="media-item-name" style="font-size: 10px; color: #999;">
            ${media.duration.toFixed(1)}s | ${media.width}x${media.height}
        </div>
    `;
    
    item.querySelector('video').src = media.video.src;
    
    item.addEventListener('dragstart', (e) => {
        e.dataTransfer.setData('mediaId', media.id);
    });
    
    mediaList.appendChild(item);
}

// Export functionality
function setupExport() {
    document.getElementById('exportBtn').addEventListener('click', () => {
        document.getElementById('exportModal').classList.add('active');
    });
    
    document.getElementById('startExportBtn').addEventListener('click', () => {
        startExport();
    });
    
    document.getElementById('newProjectBtn').addEventListener('click', () => {
        if (confirm('Start a new project? All unsaved changes will be lost.')) {
            window.location.reload();
        }
    });
    
    document.getElementById('addTextBtn').addEventListener('click', () => {
        window.timeline.addTextLayer();
    });
}

async function startExport() {
    const format = document.getElementById('exportFormat').value;
    const quality = document.getElementById('exportQuality').value;
    
    document.getElementById('startExportBtn').disabled = true;
    document.getElementById('exportProgress').style.display = 'block';
    document.getElementById('exportStatus').textContent = 'Preparing export...';
    document.getElementById('downloadSection').style.display = 'none';
    
    try {
        if (!ffmpeg) {
            await loadFFmpeg();
        }
        
        const clips = window.timeline.clips;
        if (clips.length === 0) {
            throw new Error('No clips to export! Add videos to timeline first.');
        }
        
        console.log('🎬 Starting export with', clips.length, 'clips');
        
        document.getElementById('exportStatus').textContent = 'Writing video files...';
        updateProgress(20);
        
        // Write all clips to FFmpeg filesystem
        for (let i = 0; i < clips.length; i++) {
            const clip = clips[i];
            const data = new Uint8Array(await clip.media.file.arrayBuffer());
            ffmpeg.FS('writeFile', `input${i}.mp4`, data);
            console.log(`✓ Written clip ${i}: ${clip.media.filename}`);
        }
        
        document.getElementById('exportStatus').textContent = 'Building video processing pipeline...';
        updateProgress(30);
        
        // Build complex FFmpeg filter chain
        const filterComplex = buildFilterComplex(clips);
        console.log('🎨 Filter complex:', filterComplex);
        
        // Build FFmpeg arguments
        const ffmpegArgs = [];
        
        // Add all inputs
        for (let i = 0; i < clips.length; i++) {
            ffmpegArgs.push('-i', `input${i}.mp4`);
        }
        
        // Add filter complex
        ffmpegArgs.push('-filter_complex', filterComplex);
        
        // Map output
        ffmpegArgs.push('-map', '[final]');
        
        // Quality settings
        if (quality === 'high') {
            ffmpegArgs.push('-c:v', 'libx264', '-preset', 'medium', '-crf', '18');
        } else if (quality === 'medium') {
            ffmpegArgs.push('-c:v', 'libx264', '-preset', 'fast', '-crf', '23');
        } else {
            ffmpegArgs.push('-c:v', 'libx264', '-preset', 'veryfast', '-crf', '28');
        }
        
        // Output format
        if (format === 'hls') {
            ffmpegArgs.push(
                '-hls_time', '10',
                '-hls_list_size', '0',
                '-hls_segment_filename', 'segment%03d.ts',
                '-f', 'hls',
                'output.m3u8'
            );
        } else {
            ffmpegArgs.push('output.mp4');
        }
        
        console.log('🚀 FFmpeg command:', 'ffmpeg', ...ffmpegArgs);
        
        document.getElementById('exportStatus').textContent = 'Processing video (this may take a while)...';
        updateProgress(40);
        
        // Execute FFmpeg
        await ffmpeg.run(...ffmpegArgs);
        
        updateProgress(85);
        document.getElementById('exportStatus').textContent = 'Creating download links...';
        
        // Read output files
        const files = ffmpeg.FS('readdir', '/');
        console.log('📁 Output files:', files);
        
        if (format === 'hls') {
            if (!files.includes('output.m3u8')) {
                throw new Error('HLS output not created. Check console for errors.');
            }
            
            const m3u8Data = ffmpeg.FS('readFile', 'output.m3u8');
            const m3u8Blob = new Blob([m3u8Data.buffer], { type: 'application/vnd.apple.mpegurl' });
            const m3u8Url = URL.createObjectURL(m3u8Blob);
            
            const segmentFiles = files.filter(f => f.startsWith('segment') && f.endsWith('.ts'));
            
            updateProgress(100);
            document.getElementById('exportStatus').textContent = '✅ Export complete!';
            document.getElementById('exportStatus').style.color = '#67ea67';
            
            const downloadSection = document.getElementById('downloadSection');
            downloadSection.style.display = 'block';
            downloadSection.innerHTML = `
                <h3 style="color: #67ea67; margin-bottom: 10px;">✅ Export Successful!</h3>
                <p style="color: #ccc; margin-bottom: 15px;">
                    Created ${segmentFiles.length} HLS segments with all effects applied!
                </p>
                <a href="${m3u8Url}" download="playlist.m3u8" class="download-link">
                    📄 Download playlist.m3u8
                </a>
            `;
            
            for (const segFile of segmentFiles) {
                const segData = ffmpeg.FS('readFile', segFile);
                const segBlob = new Blob([segData.buffer], { type: 'video/mp2t' });
                const segUrl = URL.createObjectURL(segBlob);
                downloadSection.innerHTML += `
                    <a href="${segUrl}" download="${segFile}" class="download-link">
                        🎞️ Download ${segFile}
                    </a>
                `;
            }
        } else {
            // MP4 output
            if (!files.includes('output.mp4')) {
                throw new Error('MP4 output not created. Check console for errors.');
            }
            
            const mp4Data = ffmpeg.FS('readFile', 'output.mp4');
            const mp4Blob = new Blob([mp4Data.buffer], { type: 'video/mp4' });
            const mp4Url = URL.createObjectURL(mp4Blob);
            
            updateProgress(100);
            document.getElementById('exportStatus').textContent = '✅ Export complete!';
            document.getElementById('exportStatus').style.color = '#67ea67';
            
            const downloadSection = document.getElementById('downloadSection');
            downloadSection.style.display = 'block';
            downloadSection.innerHTML = `
                <h3 style="color: #67ea67; margin-bottom: 10px;">✅ Export Successful!</h3>
                <p style="color: #ccc; margin-bottom: 15px;">
                    Video processed with all effects applied!
                </p>
                <a href="${mp4Url}" download="edited_video.mp4" class="download-link">
                    🎬 Download Video (MP4)
                </a>
            `;
        }
        
        console.log('✅ Export completed successfully!');
        
        // Cleanup
        for (let i = 0; i < clips.length; i++) {
            try {
                ffmpeg.FS('unlink', `input${i}.mp4`);
            } catch (e) {}
        }
        
    } catch (error) {
        console.error('❌ Export error:', error);
        document.getElementById('exportStatus').textContent = '❌ Export failed';
        document.getElementById('exportStatus').style.color = '#ff4444';
        
        // Get FFmpeg errors if available
        const ffmpegErrorLog = window.ffmpegErrors && window.ffmpegErrors.length > 0 
            ? window.ffmpegErrors.slice(-10).join('\n') 
            : 'No FFmpeg error logs captured';
        
        const downloadSection = document.getElementById('downloadSection');
        downloadSection.style.display = 'block';
        downloadSection.innerHTML = `
            <h3 style="color: #ff4444; margin-bottom: 10px;">❌ Export Failed</h3>
            <p style="color: #ccc; margin-bottom: 15px; font-size: 14px;">
                ${error.message}
            </p>
            <details style="color: #999; font-size: 12px; margin-top: 10px;" open>
                <summary style="cursor: pointer; font-weight: bold; margin-bottom: 10px;">🔍 FFmpeg Error Log (Last 10 lines)</summary>
                <pre style="margin-top: 10px; padding: 10px; background: #1a1a1a; border-radius: 5px; overflow-x: auto; color: #ff6b6b; font-size: 11px; line-height: 1.4; max-height: 300px; overflow-y: auto;">${ffmpegErrorLog}</pre>
            </details>
            <details style="color: #999; font-size: 12px; margin-top: 10px;">
                <summary style="cursor: pointer;">💡 Troubleshooting Tips</summary>
                <ul style="margin-top: 10px; padding-left: 20px; line-height: 1.8;">
                    <li><strong>Check your effects:</strong> Try removing all effects and export with defaults</li>
                    <li><strong>Try one clip:</strong> Remove extra clips and try exporting just one</li>
                    <li><strong>Remove text:</strong> Text overlays can cause filter issues</li>
                    <li><strong>Use MP4 format:</strong> Try changing export format to MP4</li>
                    <li><strong>Simplify:</strong> Start with a basic clip with no trim/effects</li>
                    <li><strong>Browser console:</strong> Press F12 and check for red errors</li>
                </ul>
            </details>
        `;
        
        // Clear FFmpeg errors for next attempt
        window.ffmpegErrors = [];
    }
    
    document.getElementById('startExportBtn').disabled = false;
}

function buildFilterComplex(clips) {
    let filter = '';
    
    // Process each clip individually
    for (let i = 0; i < clips.length; i++) {
        const clip = clips[i];
        
        filter += `[${i}:v]`;
        
        // TRIM
        const trimStart = clip.startTime || 0;
        const trimEnd = clip.endTime || clip.duration;
        filter += `trim=start=${trimStart}:end=${trimEnd},setpts=PTS-STARTPTS`;
        
        // Apply effects if they exist
        if (clip.effects) {
            // CROP
            if (clip.effects.crop) {
                const c = clip.effects.crop;
                filter += `,crop=${c.width}:${c.height}:${c.x}:${c.y}`;
            }
            
            // SCALE
            if (clip.effects.scale) {
                filter += `,scale=${clip.effects.scale.width}:${clip.effects.scale.height}`;
            }
            
            // FLIP
            if (clip.effects.flip === 'horizontal') {
                filter += `,hflip`;
            } else if (clip.effects.flip === 'vertical') {
                filter += `,vflip`;
            }
            
            // ROTATE (only if not 0)
            if (clip.effects.rotate && clip.effects.rotate !== 0) {
                const radians = clip.effects.rotate * Math.PI / 180;
                filter += `,rotate=${radians}`;
            }
            
            // COLOR ADJUSTMENTS (only if changed from defaults)
            const brightness = clip.effects.brightness || 0;
            const contrast = clip.effects.contrast || 1;
            const saturation = clip.effects.saturation || 1;
            
            if (brightness !== 0 || contrast !== 1 || saturation !== 1) {
                filter += `,eq=brightness=${brightness}:contrast=${contrast}:saturation=${saturation}`;
            }
            
            // BLUR (only if > 0)
            if (clip.effects.blur && clip.effects.blur > 0) {
                filter += `,boxblur=${clip.effects.blur}`;
            }
            
            // SHARPEN (only if > 0)
            if (clip.effects.sharpen && clip.effects.sharpen > 0) {
                filter += `,unsharp=5:5:${clip.effects.sharpen}:5:5:0`;
            }
            
            // FADE IN (only if > 0)
            if (clip.effects.fadeIn && clip.effects.fadeIn > 0) {
                filter += `,fade=t=in:st=0:d=${clip.effects.fadeIn}`;
            }
            
            // FADE OUT (only if > 0)
            if (clip.effects.fadeOut && clip.effects.fadeOut > 0) {
                const duration = trimEnd - trimStart;
                const fadeOutStart = duration - clip.effects.fadeOut;
                if (fadeOutStart > 0) {
                    filter += `,fade=t=out:st=${fadeOutStart}:d=${clip.effects.fadeOut}`;
                }
            }
        }
        
        // Ensure scale to standard size for compatibility
        filter += `,scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2`;
        
        filter += `[v${i}]`;
        
        // Add semicolon if not the last clip OR if we'll have more filters after
        if (i < clips.length - 1) {
            filter += ';';
        }
    }
    
    // Concatenate clips if multiple
    if (clips.length > 1) {
        filter += ';'; // Add semicolon before concat
        for (let i = 0; i < clips.length; i++) {
            filter += `[v${i}]`;
        }
        filter += `concat=n=${clips.length}:v=1:a=0[vconcat]`;
    } else {
        // Single clip, just use v0 as vconcat
        filter += ';[v0]copy[vconcat]';
    }
    
    // Add text layers if any
    if (window.timeline.textLayers && window.timeline.textLayers.length > 0) {
        let currentLabel = 'vconcat';
        
        for (let i = 0; i < window.timeline.textLayers.length; i++) {
            const text = window.timeline.textLayers[i];
            const nextLabel = i === window.timeline.textLayers.length - 1 ? 'final' : `vtext${i}`;
            
            // Escape text for FFmpeg - replace special characters
            const escapedText = text.text.replace(/'/g, "'\\\\\\''").replace(/:/g, '\\:');
            
            filter += `;[${currentLabel}]drawtext=text='${escapedText}':x=${text.x}:y=${text.y}:fontsize=${text.fontSize}:fontcolor=${text.color}:enable='between(t\\,${text.startTime}\\,${text.endTime})'[${nextLabel}]`;
            
            currentLabel = nextLabel;
        }
    } else {
        // No text, just copy to final
        filter += ';[vconcat]copy[final]';
    }
    
    // Remove any trailing semicolon
    filter = filter.replace(/;$/, '');
    
    return filter;
}

async function loadFFmpeg() {
    if (ffmpeg) return;
    
    document.getElementById('exportStatus').textContent = 'Loading FFmpeg engine...';
    updateProgress(5);
    
    ffmpeg = FFmpeg.createFFmpeg({ 
        log: true,
        corePath: '../ffmpeg-core/ffmpeg-core.js'
    });
    
    // Store FFmpeg errors
    window.ffmpegErrors = [];
    
    ffmpeg.setLogger(({ type, message }) => {
        console.log(`[FFmpeg ${type}]`, message);
        
        // Capture errors
        if (type === 'fferr') {
            window.ffmpegErrors.push(message);
            
            // Show critical errors immediately
            if (message.toLowerCase().includes('error') || 
                message.toLowerCase().includes('invalid') ||
                message.toLowerCase().includes('failed')) {
                console.error('🔴 FFmpeg Error:', message);
            }
        }
    });
    
    ffmpeg.setProgress(({ ratio }) => {
        if (ratio > 0 && ratio < 1) {
            const percentage = Math.round(40 + ratio * 45);
            updateProgress(percentage);
        }
    });
    
    await ffmpeg.load();
    console.log('✅ FFmpeg loaded');
}

async function loadEditorModule() {
    try {
        editorModule = await createEditorModule();
        window.editorModule = editorModule;
        editorModule.ccall('init_project', null, ['number', 'number', 'number'], [1920, 1080, 30]);
        console.log('✅ Editor C module loaded');
    } catch (error) {
        console.warn('⚠️  C module not available:', error);
    }
}

function updateProgress(percentage) {
    document.getElementById('exportProgressFill').style.width = percentage + '%';
}