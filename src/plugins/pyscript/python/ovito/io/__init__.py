""" This module contains functions and classes related to file input and output.

It primarily provides two high-level functions for reading and writing
external files:

    * :py:func:`import_file`
    * :py:func:`export_file`

In addition, it contains the :py:class:`FileSource` class, which is a data source object
that reads its input data from an external file.
"""

import ovito.data
import PyScriptScene

# Load the native module.
from PyScriptFileIO import *

def import_file(location, **params):
    """ This high-level function imports an external data file. 
    
        This Python function corresponds to the *Open Local File* command in OVITO's
        user interface. The format of the imported file is automatically detected.
        However, depending on the file's format, additional keyword parameters may need to be supplied to 
        the file parser to specify how the data should be interpreted. 
        These keyword parameters are documented below.
        
        The function creates a new :py:class:`~ovito.ObjectNode` and adds it to the current scene.
        Thus, the imported dataset will appear as an additional object in the viewports. You can remove the node 
        from the scene again by calling its :py:meth:`~ovito.ObjectNode.remove_from_scene` method.
        
        :param str location: The file to import. This can be a local file path or a remote sftp:// URL.
        :returns: The :py:class:`~ovito.ObjectNode` that has been created for the imported data.
                  
        **File columns**
        
        When importing XYZ files or binary LAMMPS dump files, the mapping of file columns 
        to OVITO's particle properties must be specified using the ``columns`` keyword parameter::
        
            import_file("file.xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
        
        The length of the list must match the number of columns in the input file. To ignore a column 
        during import, specify ``None`` instead of a property name at the corresponding position in the list.
        
        **Multi-timestep files**
        
        Some data formats can store multiple frames in a single file (e.g. XYZ and LAMMPS dump). OVITO cannot know 
        that such a file contains multiple frames (reading the entire file is avoided for performance reasons). 
        So explicitly tell OVITO to scan the entire file and load a sequence of frames by supplying the ``multiple_frames`` 
        option:: 
        
            node = import_file("file.dump", multiple_frames = True)
            print "Number of frames:", node.source.num_frames
            
    """
    
    # Determine the file's format.
    importer = FileImporter.autodetectFileFormat(ovito.dataset, location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")
    
    # Forward user parameters to the file importer object.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Importer object %s has no attribute '%s'." % (importer, key))
        importer.__setattr__(key, params[key])

    # Import data.
    if not importer.importFile(location, ImportMode.AddToScene):
        raise RuntimeError("Operation has been canceled by the user.")

    # Get the newly created ObjectNode.
    node = ovito.dataset.selected_node
    if not isinstance(node, ovito.ObjectNode):
        raise RuntimeError("File import failed. Nothing was imported.")
    
    try:
        # Block execution until file is loaded.
        # Raise exception if error occurs during loading, or if canceled by the user.
        if not node.wait(signalError = True):
            raise RuntimeError("Operation has been canceled by the user.")
    except:
        # Delete newly created scene node when import failed.
        node.delete()
        raise
    
    return node    

def _FileSource_load(self, location, **params):
    """ Loads a new external file into this source object.
    
        The function auto-detects the format of the file.
        
        The function accepts additional keyword arguments that are forwarded to the format-specific file importer.
        See the documentation of the :py:func:`import_file` function for more information.

        :param str location: The local file or remote sftp:// URL to load.
    """

    # Determine the file's format.
    importer = FileImporter.autodetectFileFormat(self.dataset, location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")
    
    # Re-use existing importer if compatible.
    if self.importer != None and type(self.importer) == type(importer):
        importer = self.importer
        
    # Forward user parameters to the importer.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Importer object %s does not have an attribute '%s'." % (importer, key))
        importer.__setattr__(key, params[key])

    # Load new data file.
    if not self.setSource(location, importer, True):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Block execution until data has been loaded. 
    if not self.waitUntilReady(self.dataset.anim.time, "Script is waiting for I/O operation to finish."):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Raise Python error if loading failed.
    if self.status.type == PyScriptScene.PipelineStatus.Type.Error:
        raise RuntimeError(self.status.text)
    
FileSource.load = _FileSource_load

def _FileSource_data(self):
    """ A :py:class:`~ovito.data.DataCollection` containing the input data that was read from the external file. """

    # Block execution until data has been fully loaded. 
    if not self.waitUntilReady(self.dataset.anim.time, "Script is waiting for I/O operation to finish."):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Get data objects.
    data = self.evaluate(self.dataset.anim.time)
    
    # Raise Python error if loading failed.
    if data.status.type == PyScriptScene.PipelineStatus.Type.Error:
        raise RuntimeError(data.status.text)
    
    return data
    
FileSource.data = property(_FileSource_data)

# Implement the 'sourceUrl' property of FileSource, which returns or sets the currently loaded file path.
def _get_FileSource_source_path(self, _originalGetterMethod = FileSource.source_path):
    """ The path or URL of the loaded file. """    
    return _originalGetterMethod.__get__(self)
def _set_FileSource_source_path(self, url):
    """ Sets the URL of the file referenced by this FileSource. """
    self.setSource(url, None) 
FileSource.source_path = property(_get_FileSource_source_path, _set_FileSource_source_path)

def export_file(node, file, format, **params):
    """ High-level function that exports data to a file.
    
        :param node: The node that provides the data to be exported.
        :type node: :py:class:`~ovito.scene.ObjectNode` 
        :param str file: The name of the output file.
        :param str format: The type of file to write:
        
                            * ``"lammps_dump"`` -- LAMMPS text-based dump format
                            * ``"lammps_data"`` -- LAMMPS data format
                            * ``"imd"`` -- IMD format
                            * ``"vasp"`` -- POSCAR format
                            * ``"xyz"`` -- XYZ format
        
        The function evaluates the modification pipeline of the given object node and exports
        the results to one or more files. By default, only the current animation frame is exported.
        
        Depending on the selected export format, additional keyword parameters need to be specified.
       
        **File columns**
        
        When writing files in the ``"lammps_dump"`` or ``"xyz"`` formats, you must specify the particle properties to be exported 
        using the ``columns`` keyword parameter::
        
            export_file(node, "output.xyz", "xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"]
            )
            
        When exporting to a ``imd`` file, the ``columns`` keyword is optional and can be used to export extra particle properties
        in addition to the standard columns defined by the IMD file format (e.g. positions, mass, etc.)
    """
    
    # Look up the exporter class for the selected format.
    if not format in export_file._formatTable:
        raise RuntimeError("Unknown output file format: %s" % format)
    
    # Create an instance of the exporter class.
    exporter = export_file._formatTable[format](params)
    
    # Ensure the data to be exported is available.
    if not node.wait():
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Export data.
    if not exporter.exportToFile([node], file, True):
        raise RuntimeError("Operation has been canceled by the user.")

# This is the table of export formats used by the export_file() function
# to look up the right exporter class for a file format.
# Plugins can register their exporter class by inserting a new entry in this dictionary.
export_file._formatTable = {}
    