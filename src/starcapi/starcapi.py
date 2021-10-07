from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename
import os
import subprocess
import json

def run_starc_api(arguments):
    xdg_runtime_dir = '/tmp/starcapi'
    if not os.path.exists(xdg_runtime_dir):
        os.makedirs(xdg_runtime_dir)
    binary_path = os.path.abspath(os.path.dirname(__file__)) + '/bin/'
    starc_api_command = 'XDG_RUNTIME_DIR=' + xdg_runtime_dir \
                        + ' LD_LIBRARY_PATH=' + binary_path \
                        + ' QT_QPA_PLATFORM=offscreen ' \
                        + binary_path + 'starcapi'
    output = subprocess.Popen(starc_api_command + ' ' + arguments,
                              shell=True, stderr=subprocess.PIPE).stderr.read()
    return json.loads(output)

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = '/tmp/'

starc_api_base_route = '/api/v1/'

@app.route(starc_api_base_route + '/stories/', methods=['POST'])
def stories():
    # check if the post request has the file part
    documents = request.files.getlist("documents")
    filenames = []
    for document in documents:
        if document.filename != '' and document:
            filename = secure_filename(document.filename)
            document.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))
            filenames.append(filename)
    # If the user does not select a file, the browser submits an
    # empty file without a filename.
    if not filenames:
        return 'No document part', 400
    # Run starc API to processing saved document
    arguments = 'stories'
    for filename in filenames:
        arguments += ' ' + filename
    return jsonify(run_starc_api(arguments))

@app.route(starc_api_base_route + '/stories/<story>/characters/')
def characters(story):
    arguments = 'characters ' + story
    return jsonify(run_starc_api(arguments))

@app.route(starc_api_base_route + '/stories/<story>/characters/<character>/')
def character(story, character):
    arguments = 'character ' + story + ' "' + character + '"'
    return jsonify(run_starc_api(arguments))

if __name__ == '__main__':
    app.run()
