// web_files.js - browser side of the file bridge. Linked with --js-library.
//
// The user picks their EverQuest folder with a directory <input>. We keep the
// File objects in memory (nothing is uploaded anywhere) and copy only the
// files for the zone being opened into the virtual file system at /eq.

addToLibrary({
  $MeshGenFiles__postset: 'Module["setEverQuestFiles"] = function (files, folder) { MeshGenFiles.setFiles(files, folder); };',
  $MeshGenFiles: {
    files: null,        // Map: lower case file name -> File
    folderName: '',
    loadedZoneFiles: [],

    ensureDir: function (path) {
      var parts = path.split('/').filter(Boolean);
      var cur = '';
      for (var i = 0; i < parts.length; i++) {
        cur += '/' + parts[i];
        try { FS.mkdir(cur); } catch (e) { /* exists */ }
      }
    },

    // Zone file names that belong to a zone short name.
    zoneFileNames: function (zone) {
      var out = [];
      var files = MeshGenFiles.files;
      if (!files) return out;
      files.forEach(function (file, name) {
        if (name === zone + '.s3d' || name === zone + '.eqg' ||
            name.startsWith(zone + '_') || name.startsWith(zone + '.')) {
          out.push(name);
        }
      });
      return out;
    },

    // Takes a FileList / array of File objects from the EverQuest folder.
    setFiles: function (fileList, folderName) {
      var map = new Map();
      var folder = folderName || '';
      for (var i = 0; i < fileList.length; i++) {
        var f = fileList[i];
        // FileSystemFileHandle (directory picker) - already top level only.
        if (f.kind === 'file' && f.getFile) {
          map.set(f.name.toLowerCase(), f);
          continue;
        }
        var rel = f.webkitRelativePath || f.name;
        var parts = rel.split('/');
        if (!folder && parts.length > 1) folder = parts[0];
        // Only files directly in the EverQuest folder are used.
        if (parts.length > 2) continue;
        map.set(f.name.toLowerCase(), f);
      }
      MeshGenFiles.files = map;
      MeshGenFiles.folderName = folder;
      var names = Array.from(map.keys()).join('\n');
      Module.ccall('meshgen_files_ready', null, ['string', 'string'], [folder, names]);
      if (Module.onFolderPicked) Module.onFolderPicked(folder, map.size);
    },

    // Accepts a File or a FileSystemFileHandle.
    getFile: function (f) {
      return (f.getFile && f.kind === 'file') ? f.getFile() : Promise.resolve(f);
    },

    readFile: function (f) {
      return MeshGenFiles.getFile(f).then(function (file) { return file.arrayBuffer(); })
        .then(function (buf) { return new Uint8Array(buf); });
    },
  },

  meshgen_js_pick_folder__deps: ['$MeshGenFiles'],
  meshgen_js_pick_folder: function () {
    // Chrome/Edge: the File System Access API asks to "view files" and only
    // lists the top folder. Files are opened lazily when a zone is loaded.
    if (window.showDirectoryPicker) {
      window.showDirectoryPicker({ id: 'everquest', mode: 'read' }).then(async function (dir) {
        var handles = [];
        for await (var entry of dir.values()) {
          if (entry.kind === 'file') handles.push(entry);
        }
        MeshGenFiles.setFiles(handles, dir.name);
      }).catch(function (err) {
        if (err && err.name !== 'AbortError') console.error('Folder picker failed', err);
      });
      return;
    }
    // Other browsers: directory <input>. Its prompt says "upload", but the
    // files are only read locally.
    var input = document.createElement('input');
    input.type = 'file';
    input.webkitdirectory = true;
    input.multiple = true;
    input.style.display = 'none';
    input.addEventListener('change', function () {
      MeshGenFiles.setFiles(input.files);
      document.body.removeChild(input);
    });
    document.body.appendChild(input);
    input.click();
  },

  meshgen_js_request_zone__deps: ['$MeshGenFiles'],
  meshgen_js_request_zone: function (zonePtr) {
    var zone = UTF8ToString(zonePtr).toLowerCase();
    MeshGenFiles.ensureDir('/eq');

    // Free the previous zone's files.
    MeshGenFiles.loadedZoneFiles.forEach(function (name) {
      try { FS.unlink('/eq/' + name); } catch (e) {}
    });
    MeshGenFiles.loadedZoneFiles = [];

    var names = MeshGenFiles.zoneFileNames(zone);
    var files = MeshGenFiles.files;

    var copyFile = function (name) {
      var file = files.get(name);
      if (!file) return Promise.resolve();
      return MeshGenFiles.readFile(file).then(function (data) {
        FS.writeFile('/eq/' + name, data);
        MeshGenFiles.loadedZoneFiles.push(name);
      });
    };

    // EQG zones list extra archives in <zone>_assets.txt.
    var loadAssets = function () {
      if (!files || !files.has(zone + '_assets.txt')) return Promise.resolve();
      return MeshGenFiles.getFile(files.get(zone + '_assets.txt')).then(function (f) { return f.text(); }).then(function (text) {
        var extra = text.split(/\s+/).map(function (n) { return n.trim().toLowerCase(); })
          .filter(function (n) { return n && files.has(n) && names.indexOf(n) < 0; });
        return Promise.all(extra.map(copyFile));
      });
    };

    Promise.all(names.map(copyFile)).then(loadAssets).then(function () {
      Module.ccall('meshgen_open_zone', null, ['string'], [zone]);
    }).catch(function (err) {
      console.error('Failed to read zone files', err);
      Module.ccall('meshgen_open_zone', null, ['string'], [zone]);
    });
  },

  meshgen_js_pick_navmesh__deps: ['$MeshGenFiles'],
  meshgen_js_pick_navmesh: function (destPtr) {
    var dest = UTF8ToString(destPtr);
    var input = document.createElement('input');
    input.type = 'file';
    input.accept = '.navmesh';
    input.style.display = 'none';
    input.addEventListener('change', function () {
      var file = input.files[0];
      document.body.removeChild(input);
      if (!file) return;
      MeshGenFiles.readFile(file).then(function (data) {
        MeshGenFiles.ensureDir(dest.substring(0, dest.lastIndexOf('/')));
        FS.writeFile(dest, data);
        Module.ccall('meshgen_navmesh_ready', null, [], []);
      });
    });
    document.body.appendChild(input);
    input.click();
  },

  meshgen_js_download: function (pathPtr, namePtr) {
    var path = UTF8ToString(pathPtr);
    var name = UTF8ToString(namePtr);
    try {
      var data = FS.readFile(path);
      var blob = new Blob([data], { type: 'application/octet-stream' });
      var url = URL.createObjectURL(blob);
      var a = document.createElement('a');
      a.href = url;
      a.download = name;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      setTimeout(function () { URL.revokeObjectURL(url); }, 10000);
    } catch (e) {
      console.error('Download failed', e);
    }
  },
});
